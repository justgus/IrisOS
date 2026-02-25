#pragma once

#include "refract/schema_registry.h"

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

private:
  SchemaRegistry& registry_;
  InheritanceResolver resolver_;
};

} // namespace iris::refract
