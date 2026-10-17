#include "refract/contract_metadata.h"
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace iris::refract {
namespace {
void require(bool value, const char* message) {
  if (!value) throw std::invalid_argument(message);
}
referee::ObjectID identity(const nlohmann::json& json) {
  const auto s = json.get<std::string>();
  require(s.size() == 32 && s.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos,
          "invalid binding/contract identity");
  auto id = referee::ObjectID::from_hex(s);
  require(id != referee::ObjectID{}, "zero binding/contract identity");
  return id;
}
std::uint64_t positive(const nlohmann::json& json) {
  require(json.is_number_unsigned(), "expected positive integer");
  const auto n = json.get<std::uint64_t>();
  require(n > 0, "zero version/type"); return n;
}
}
void encode_contract_metadata(nlohmann::json& j, const OperationDefinition& op) {
  if (!op.conformances.empty()) {
    j["conformances"] = nlohmann::json::array();
    for (const auto& c : op.conformances) {
      nlohmann::json substitutions = nlohmann::json::object();
      for (const auto& [name, type] : c.substitutions) substitutions[name] = type.v;
      j["conformances"].push_back({{"id", c.contract.id.to_hex()},
                                  {"version", c.contract.version}, {"substitutions", substitutions}});
    }
  }
  if (op.binding) {
    const auto& b = *op.binding;
    const auto route = b.route == ExecutionRoute::Local ? "local"
        : b.route == ExecutionRoute::Service ? "service" : "object_system";
    j["binding"] = {{"id", b.implementation.to_hex()}, {"version", b.version},
                     {"route", route}, {"destination", b.destination}};
  }
}
referee::Result<void> decode_contract_metadata(const nlohmann::json& j, OperationDefinition& op) {
  try {
    if (j.contains("conformances")) {
      require(j.at("conformances").is_array(), "conformances must be an array");
      std::set<std::pair<std::string, std::uint64_t>> keys;
      for (const auto& item : j.at("conformances")) {
        ContractConformance c;
        c.contract = {identity(item.at("id")), positive(item.at("version"))};
        require(keys.insert({c.contract.id.to_hex(), c.contract.version}).second, "duplicate conformance");
        require(item.at("substitutions").is_object(), "substitutions must be an object");
        for (const auto& [name, value] : item.at("substitutions").items()) {
          require(!name.empty(), "empty substitution name");
          c.substitutions[name] = {positive(value)};
        }
        op.conformances.push_back(std::move(c));
      }
    }
    if (j.contains("binding")) {
      const auto& item = j.at("binding");
      OperationBinding b;
      b.implementation = identity(item.at("id")); b.version = positive(item.at("version"));
      const auto route = item.at("route").get<std::string>();
      require(route == "local" || route == "service" || route == "object_system", "unknown route");
      b.route = route == "local" ? ExecutionRoute::Local
          : route == "service" ? ExecutionRoute::Service : ExecutionRoute::ObjectSystem;
      b.destination = item.at("destination").get<std::string>();
      require((b.route == ExecutionRoute::Local) == b.destination.empty(), "invalid route destination");
      op.binding = std::move(b);
    }
    require(op.conformances.empty() == !op.binding.has_value(), "conformance/binding must occur together");
    if (!op.conformances.empty()) {
      const auto scope = j.at("scope").get<std::string>();
      require(scope == "object" || scope == "class", "invalid conforming operation scope");
      const auto& signature = j.at("signature");
      require(signature.is_object(), "conforming signature must be an object");
      for (const auto* field : {"params", "outputs"}) {
        require(signature.at(field).is_array(), "signature parameters must be an array");
        for (const auto& p : signature.at(field)) {
          require(!p.at("name").get<std::string>().empty(), "empty parameter name");
          positive(p.at("type_id"));
          require(p.at("optional").is_boolean(), "optionality must be boolean");
        }
      }
      if (j.contains("effects")) require(j.at("effects").is_array(), "effects must be an array");
    }
    return referee::Result<void>::ok();
  } catch (const std::exception& ex) {
    return referee::Result<void>::err(referee::ErrorCode::CorruptData, ex.what());
  }
}
}
