#pragma once

#include "refract/schema_registry.h"
#include "refract/contract_conformance.h"

#include <functional>
#include <vector>

namespace iris::refract {

class OperationRegistry {
public:
  using InheritanceResolver = std::function<std::vector<referee::TypeID>(referee::TypeID)>;

  explicit OperationRegistry(SchemaRegistry& registry,
                             InheritanceResolver resolver = {});

  referee::Result<std::vector<OperationDefinition>> list_operations(
      referee::TypeID type,
      OperationScope scope,
      bool include_inherited = true);

  referee::Result<std::vector<ConformingOperation>> list_conformances(
      referee::TypeID type, OperationCatalog& catalog,
      std::optional<ContractKey> contract = {}, bool include_inherited = true);

  referee::Result<std::vector<ConformingOperation>> list_object_conformances(
      referee::ObjectID object, referee::SqliteStore& store, OperationCatalog& catalog,
      std::optional<ContractKey> contract = {});

private:
  SchemaRegistry& registry_;
  InheritanceResolver resolver_;
};

} // namespace iris::refract
