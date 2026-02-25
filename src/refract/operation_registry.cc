#include "refract/operation_registry.h"

#include <cstdint>
#include <deque>
#include <unordered_set>

namespace iris::refract {

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

    if (include_inherited && resolver_) {
      for (const auto& base : resolver_(current)) {
        if (visited.insert(base.v).second) {
          queue.push_back(base);
        }
      }
    }
  }

  return referee::Result<std::vector<OperationDefinition>>::ok(std::move(out));
}

} // namespace iris::refract
