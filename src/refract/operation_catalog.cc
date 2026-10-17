#include "refract/operation_catalog.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <set>
#include <stdexcept>

namespace iris::refract {
namespace {
using Json = nlohmann::json;

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

bool nonzero(referee::ObjectID id) { return id != referee::ObjectID{}; }

void validate(const OperationContract& c) {
  require(nonzero(c.key.id) && c.key.version > 0, "invalid contract identity/version");
  require(!c.name.empty() && !c.category.empty(), "contract name/category is empty");
  require(c.scope == OperationScope::Object || c.scope == OperationScope::Class, "invalid scope");
  require(c.documentation.summary && !c.documentation.summary->empty(), "contract behavior is undocumented");
  std::set<std::string> parameters;
  for (const auto& p : c.type_parameters) {
    require(!p.empty() && parameters.insert(p).second, "invalid duplicate type parameter");
  }
  for (const auto* group : {&c.inputs, &c.outputs}) {
    std::set<std::string> names;
    bool optional_seen = false;
    for (const auto& p : *group) {
      require(!p.name.empty() && names.insert(p.name).second, "invalid duplicate parameter name");
      require((p.type.v != 0) != !p.type_parameter.empty(), "type expression must be concrete or parameterized");
      require(p.type_parameter.empty() || parameters.count(p.type_parameter), "unknown type parameter");
      require(!optional_seen || p.optional, "optional parameters must be trailing");
      optional_seen = p.optional;
    }
  }
  for (const auto& cap : c.required_capabilities) require(!cap.empty(), "empty capability");
  for (const auto& effect : c.effects) {
    require(effect.kind >= OperationEffectKind::Reads && effect.kind <= OperationEffectKind::Custom,
            "invalid effect kind");
  }
}

Json encode(const OperationContract& c) {
  Json j = {{"id", c.key.id.to_hex()}, {"version", c.key.version}, {"name", c.name},
            {"category", c.category}, {"scope", c.scope == OperationScope::Object ? "object" : "class"},
            {"type_parameters", c.type_parameters}, {"inputs", Json::array()},
            {"outputs", Json::array()}, {"summary", c.documentation.summary.value_or("")},
            {"examples", c.documentation.examples}, {"effects", Json::array()},
            {"capabilities", c.required_capabilities}};
  for (auto pair : {std::pair{"inputs", &c.inputs}, std::pair{"outputs", &c.outputs}}) {
    for (const auto& p : *pair.second) {
      j[pair.first].push_back({{"name", p.name}, {"type", p.type.v},
                              {"parameter", p.type_parameter}, {"optional", p.optional}});
    }
  }
  for (const auto& e : c.effects) {
    j["effects"].push_back({{"kind", static_cast<int>(e.kind)}, {"target", e.target},
                            {"description", e.description ? Json(*e.description) : Json(nullptr)}});
  }
  return j;
}

std::uint64_t number(const Json& j) {
  require(j.is_number_unsigned(), "expected unsigned integer");
  return j.get<std::uint64_t>();
}
std::vector<std::string> strings(const Json& j) {
  require(j.is_array(), "expected string array");
  std::vector<std::string> out;
  for (const auto& item : j) {
    require(item.is_string(), "expected string");
    out.push_back(item.get<std::string>());
  }
  return out;
}
OperationContract decode(const referee::Bytes& payload) {
  const auto j = Json::from_cbor(payload);
  OperationContract c;
  const auto id = j.at("id").get<std::string>();
  require(id.size() == 32 && id.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos,
          "invalid contract id");
  c.key = {referee::ObjectID::from_hex(id), number(j.at("version"))};
  c.name = j.at("name").get<std::string>();
  c.category = j.at("category").get<std::string>();
  const auto scope = j.at("scope").get<std::string>();
  require(scope == "object" || scope == "class", "invalid scope");
  c.scope = scope == "object" ? OperationScope::Object : OperationScope::Class;
  c.type_parameters = strings(j.at("type_parameters"));
  c.documentation.summary = j.at("summary").get<std::string>();
  c.documentation.examples = strings(j.at("examples"));
  c.required_capabilities = strings(j.at("capabilities"));
  for (auto pair : {std::pair{"inputs", &c.inputs}, std::pair{"outputs", &c.outputs}}) {
    require(j.at(pair.first).is_array(), "expected parameters array");
    for (const auto& p : j.at(pair.first)) {
      pair.second->push_back({p.at("name").get<std::string>(), {number(p.at("type"))},
                              p.at("parameter").get<std::string>(), p.at("optional").get<bool>()});
    }
  }
  require(j.at("effects").is_array(), "expected effects array");
  for (const auto& e : j.at("effects")) {
    const auto kind = number(e.at("kind"));
    require(kind <= static_cast<unsigned>(OperationEffectKind::Custom), "invalid effect kind");
    OperationEffect effect{static_cast<OperationEffectKind>(kind), e.at("target").get<std::string>(), {}};
    if (!e.at("description").is_null()) effect.description = e.at("description").get<std::string>();
    c.effects.push_back(std::move(effect));
  }
  validate(c);
  return c;
}
} // namespace

referee::Result<std::vector<OperationContract>> OperationCatalog::list(
    std::optional<std::string> category, std::optional<std::string> capability) {
  auto records = store_.list_by_type(kOperationContractType);
  if (!records) return referee::Result<std::vector<OperationContract>>::err(*records.error);
  std::vector<OperationContract> out;
  try {
    std::set<std::pair<std::string, std::uint64_t>> seen;
    for (const auto& record : *records.value) {
      auto c = decode(record.payload_cbor);
      require(seen.insert({c.key.id.to_hex(), c.key.version}).second, "duplicate catalog key");
      if (category && c.category != *category) continue;
      if (capability && std::find(c.required_capabilities.begin(), c.required_capabilities.end(),
                                  *capability) == c.required_capabilities.end()) continue;
      out.push_back(std::move(c));
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
      return std::pair{a.key.id.to_hex(), a.key.version} < std::pair{b.key.id.to_hex(), b.key.version};
    });
    return referee::Result<std::vector<OperationContract>>::ok(std::move(out));
  } catch (const std::exception& ex) {
    return referee::Result<std::vector<OperationContract>>::err(referee::ErrorCode::CorruptData, ex.what());
  }
}

referee::Result<OperationContract> OperationCatalog::get(ContractKey key) {
  if (!nonzero(key.id) || key.version == 0) return referee::Result<OperationContract>::err(
      referee::ErrorCode::InvalidArgument, "invalid contract key");
  auto all = list();
  if (!all) return referee::Result<OperationContract>::err(*all.error);
  for (auto& c : *all.value) if (c.key == key) return referee::Result<OperationContract>::ok(std::move(c));
  return referee::Result<OperationContract>::err(referee::ErrorCode::NotFound, "contract not found");
}

referee::Result<OperationContract> OperationCatalog::register_contract(const OperationContract& c) {
  try { validate(c); } catch (const std::exception& ex) {
    return referee::Result<OperationContract>::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
  auto existing = get(c.key);
  if (existing) {
    if (encode(*existing.value) == encode(c)) return existing;
    return referee::Result<OperationContract>::err(referee::ErrorCode::AlreadyExists,
                                                   "contract version is immutable");
  }
  if (existing.error->code != referee::ErrorCode::NotFound) return existing;
  const auto payload = Json::to_cbor(encode(c));
  auto created = store_.create_object(kOperationContractType, {}, payload);
  if (!created) return referee::Result<OperationContract>::err(*created.error);
  return referee::Result<OperationContract>::ok(c);
}
} // namespace iris::refract
