#include "refract/operation_registry.h"

#include <cstdint>
#include <deque>
#include <unordered_set>

namespace iris::refract {

namespace {

referee::Result<std::vector<referee::TypeID>> collect_supertypes(
    SchemaRegistry& registry,
    referee::TypeID type,
    const OperationRegistry::InheritanceResolver& resolver) {
  auto storedR = registry.list_supertypes(type);
  if (!storedR) return referee::Result<std::vector<referee::TypeID>>::err(storedR.error->message);

  std::vector<referee::TypeID> out = storedR.value.value();
  std::unordered_set<std::uint64_t> seen;
  for (const auto& parent : out) {
    seen.insert(parent.v);
  }

  if (resolver) {
    for (const auto& parent : resolver(type)) {
      if (seen.insert(parent.v).second) out.push_back(parent);
    }
  }

  return referee::Result<std::vector<referee::TypeID>>::ok(std::move(out));
}

} // namespace

OperationRegistry::OperationRegistry(SchemaRegistry& registry,
                                     InheritanceResolver resolver)
  : registry_(registry),
    resolver_(std::move(resolver)) {}

referee::Result<std::vector<OperationDefinition>> OperationRegistry::list_operations(
    referee::TypeID type,
    OperationScope scope,
    bool include_inherited) {
  std::vector<OperationDefinition> out;
  std::deque<referee::TypeID> queue;
  std::unordered_set<std::uint64_t> visited;

  queue.push_back(type);
  visited.insert(type.v);

  while (!queue.empty()) {
    auto current = queue.front();
    queue.pop_front();

    auto defR = registry_.get_latest_definition_by_type(current);
    if (!defR) return referee::Result<std::vector<OperationDefinition>>::err(defR.error->message);
    if (!defR.value->has_value()) {
      return referee::Result<std::vector<OperationDefinition>>::err("definition not found");
    }

    const auto& def = defR.value->value().definition;
    for (const auto& op : def.operations) {
      if (op.scope == scope) out.push_back(op);
    }

    if (include_inherited) {
      auto parentsR = collect_supertypes(registry_, current, resolver_);
      if (!parentsR) {
        return referee::Result<std::vector<OperationDefinition>>::err(parentsR.error->message);
      }
      for (const auto& base : parentsR.value.value()) {
        if (visited.insert(base.v).second) {
          queue.push_back(base);
        }
      }
    }
  }

  return referee::Result<std::vector<OperationDefinition>>::ok(std::move(out));
}

} // namespace iris::refract
