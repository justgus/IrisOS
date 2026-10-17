#include "refract/contract_inspection.h"
#include "refract/contract_metadata.h"
#include <nlohmann/json.hpp>

namespace iris::refract {
namespace {
using Json = nlohmann::json;
Json describe(const OperationContract& c) {
  Json out = {{"id", c.key.id.to_hex()}, {"version", c.key.version}, {"name", c.name},
              {"category", c.category}, {"scope", c.scope == OperationScope::Class ? "class" : "object"},
              {"type_parameters", c.type_parameters}, {"inputs", Json::array()}, {"outputs", Json::array()},
              {"summary", c.documentation.summary.value_or("")}, {"examples", c.documentation.examples},
              {"capabilities", c.required_capabilities}, {"effects", Json::array()}};
  for (auto pair : {std::pair{"inputs", &c.inputs}, std::pair{"outputs", &c.outputs}}) {
    for (const auto& p : *pair.second)
      out[pair.first].push_back({{"name", p.name}, {"type", p.type.v},
                                 {"parameter", p.type_parameter}, {"optional", p.optional}});
  }
  constexpr const char* kinds[] = {"reads", "writes", "emits", "schedules", "uses_io", "custom"};
  for (const auto& effect : c.effects)
    out["effects"].push_back({{"kind", kinds[static_cast<unsigned>(effect.kind)]},
                              {"target", effect.target}, {"description", effect.description.value_or("")}});
  return out;
}
}
referee::Result<std::string> inspect_contract_catalog(
    OperationCatalog& catalog, std::optional<std::string> category, std::optional<std::string> capability) {
  auto contracts = catalog.list(std::move(category), std::move(capability));
  if (!contracts) return referee::Result<std::string>::err(*contracts.error);
  Json out = Json::array();
  for (const auto& c : *contracts.value) out.push_back(describe(c));
  return referee::Result<std::string>::ok(out.dump(2));
}
referee::Result<std::string> inspect_type_contracts(
    SchemaRegistry& schema, OperationCatalog& catalog, referee::TypeID type) {
  OperationRegistry operations(schema);
  auto claims = operations.list_conformances(type, catalog);
  if (!claims) return referee::Result<std::string>::err(*claims.error);
  Json out = Json::array();
  for (const auto& claim : *claims.value) {
    auto contract = catalog.get(claim.conformance.contract);
    if (!contract) return referee::Result<std::string>::err(*contract.error);
    const auto& op = claim.operation;
    Json item = {{"owner", claim.owner.v}, {"depth", claim.depth}, {"operation", op.name},
                 {"contract", describe(*contract.value)}, {"capabilities", op.required_capabilities},
                 {"inputs", Json::array()}, {"outputs", Json::array()}, {"effects", Json::array()}};
    encode_contract_metadata(item, op);
    for (auto pair : {std::pair{"inputs", &op.signature.params}, std::pair{"outputs", &op.signature.outputs}}) {
      for (const auto& p : *pair.second)
        item[pair.first].push_back({{"name", p.name}, {"type", p.type.v}, {"optional", p.optional}});
    }
    // Operation effects may be stricter than the catalog's minimum declaration.
    for (const auto& e : op.effects)
      item["effects"].push_back({{"kind", static_cast<int>(e.kind)}, {"target", e.target},
                                 {"description", e.description.value_or("")}});
    out.push_back(std::move(item));
  }
  return referee::Result<std::string>::ok(out.dump(2));
}
}
