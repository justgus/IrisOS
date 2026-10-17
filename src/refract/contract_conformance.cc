#include "refract/contract_conformance.h"

#include <algorithm>
#include <set>

namespace iris::refract {
referee::Result<void> validate_contract_operations(
    OperationCatalog& catalog, const TypeDefinition& definition, referee::ErrorCode code) {
  auto invalid = [code](std::string message) { return referee::Result<void>::err(code, std::move(message)); };
  for (const auto& op : definition.operations) {
    if (op.conformances.empty()) {
      if (op.binding) return invalid("binding requires explicit contract conformance");
      continue;
    }
    if (!op.binding || op.binding->implementation == referee::ObjectID{} || op.binding->version == 0)
      return invalid("conforming operation requires a versioned binding");
    const auto& binding = *op.binding;
    if (binding.route != ExecutionRoute::Local && binding.route != ExecutionRoute::Service &&
        binding.route != ExecutionRoute::ObjectSystem) return invalid("invalid execution route");
    if ((binding.route == ExecutionRoute::Local) != binding.destination.empty())
      return invalid("local route has no destination; delegated route requires a destination");
    if (op.name.empty()) return invalid("conforming operation name is empty");
    std::set<std::pair<std::string, std::uint64_t>> seen;
    for (const auto& claim : op.conformances) {
      if (!seen.insert({claim.contract.id.to_hex(), claim.contract.version}).second)
        return invalid("duplicate contract conformance");
      auto contract = catalog.get(claim.contract);
      if (!contract) {
        if (contract.error->code == referee::ErrorCode::NotFound ||
            contract.error->code == referee::ErrorCode::InvalidArgument)
          return invalid("unknown contract version");
        return referee::Result<void>::err(*contract.error);
      }
      const auto& c = *contract.value;
      if (op.scope != c.scope || claim.substitutions.size() != c.type_parameters.size())
        return invalid("contract scope/substitutions mismatch");
      for (const auto& name : c.type_parameters) {
        auto it = claim.substitutions.find(name);
        if (it == claim.substitutions.end() || it->second.v == 0)
          return invalid("missing concrete contract substitution");
        if (name == "Self" && it->second != definition.type_id)
          return invalid("Self substitution must be declaring type");
      }
      for (const auto& required : c.required_capabilities) {
        if (std::find(op.required_capabilities.begin(), op.required_capabilities.end(), required)
            == op.required_capabilities.end()) return invalid("contract capability omitted");
      }
      for (const auto& effect : c.effects) {
        if (std::none_of(op.effects.begin(), op.effects.end(), [&](const auto& declared) {
              return declared.kind == effect.kind && declared.target == effect.target;
            })) return invalid("contract effect omitted");
      }
      auto matches = [&](const auto& actual, const auto& expected) {
        if (actual.size() != expected.size()) return false;
        std::set<std::string> names;
        for (std::size_t i = 0; i < actual.size(); ++i) {
          const auto type = expected[i].type_parameter.empty() ? expected[i].type
              : claim.substitutions.at(expected[i].type_parameter);
          if (actual[i].name.empty() || !names.insert(actual[i].name).second ||
              actual[i].type != type || actual[i].optional != expected[i].optional) return false;
        }
        return true;
      };
      if (!matches(op.signature.params, c.inputs) || !matches(op.signature.outputs, c.outputs))
        return invalid("contract signature mismatch");
    }
  }
  return referee::Result<void>::ok();
}
} // namespace iris::refract
