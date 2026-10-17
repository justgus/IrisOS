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
  if (!storedR) return referee::Result<std::vector<referee::TypeID>>::err(storedR.error.value());

  std::vector<referee::TypeID> out = storedR.value.value();
  std::unordered_set<std::uint64_t> seen;
  for (const auto& parent : out) {
    seen.insert(parent.v);
  }

  if (resolver) {
    try {
      for (const auto& parent : resolver(type)) {
        if (seen.insert(parent.v).second) out.push_back(parent);
      }
    } catch (const std::exception& ex) {
      return referee::Result<std::vector<referee::TypeID>>::err(
          referee::ErrorCode::Internal, ex.what());
    } catch (...) {
      return referee::Result<std::vector<referee::TypeID>>::err(
          referee::ErrorCode::Internal, "inheritance resolver failed");
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
    if (!defR) return referee::Result<std::vector<OperationDefinition>>::err(defR.error.value());

    const auto& def = defR.value->definition;
    for (const auto& op : def.operations) {
      if (op.scope == scope) out.push_back(op);
    }

    if (include_inherited) {
      auto parentsR = collect_supertypes(registry_, current, resolver_);
      if (!parentsR) {
        return referee::Result<std::vector<OperationDefinition>>::err(parentsR.error.value());
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

referee::Result<std::vector<ConformingOperation>> OperationRegistry::list_conformances(
    referee::TypeID type, OperationCatalog& catalog, std::optional<ContractKey> contract,
    bool include_inherited) {
  using Result = referee::Result<std::vector<ConformingOperation>>;
  if (contract) {
    auto known = catalog.get(*contract);
    if (!known) return Result::err(*known.error);
  }
  std::vector<ConformingOperation> out;
  std::deque<std::pair<referee::TypeID, std::size_t>> queue{{type, 0}};
  std::unordered_set<std::uint64_t> visited{type.v};
  while (!queue.empty()) {
    const auto [current, depth] = queue.front(); queue.pop_front();
    auto def = registry_.get_latest_definition_by_type(current);
    if (!def) return Result::err(*def.error);
    auto valid = validate_contract_operations(catalog, def.value->definition, referee::ErrorCode::CorruptData);
    if (!valid) return Result::err(*valid.error);
    for (const auto& op : def.value->definition.operations) {
      for (const auto& claim : op.conformances) {
        if (!contract || claim.contract == *contract) out.push_back({op, claim, current, depth});
      }
    }
    if (!include_inherited) continue;
    auto parents = collect_supertypes(registry_, current, resolver_);
    if (!parents) return Result::err(*parents.error);
    for (const auto& parent : *parents.value) {
      if (visited.insert(parent.v).second) queue.push_back({parent, depth + 1});
    }
  }
  return Result::ok(std::move(out));
}

referee::Result<std::vector<ConformingOperation>> OperationRegistry::list_object_conformances(
    referee::ObjectID object, referee::SqliteStore& store, OperationCatalog& catalog,
    std::optional<ContractKey> contract) {
  auto record = store.get_latest(object);
  if (!record) return referee::Result<std::vector<ConformingOperation>>::err(*record.error);
  return list_conformances(record.value->type, catalog, contract);
}

} // namespace iris::refract
