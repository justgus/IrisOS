#pragma once

#include "refract/operation_registry.h"

#include <string>
#include <string_view>
#include <vector>

namespace iris::refract {

struct DispatchMatch {
  OperationDefinition operation;
  referee::TypeID owner_type{};
  std::size_t depth{0};
};

class DispatchEngine {
public:
  using InheritanceResolver = OperationRegistry::InheritanceResolver;

  explicit DispatchEngine(SchemaRegistry& registry,
                          InheritanceResolver resolver = {});

  referee::Result<DispatchMatch> resolve(
      referee::TypeID target_type,
      std::string_view name,
      OperationScope scope,
      const std::vector<referee::TypeID>& arg_types,
      std::size_t arg_count,
      bool include_inherited = true);

private:
  SchemaRegistry& registry_;
  InheritanceResolver resolver_;
};

} // namespace iris::refract
