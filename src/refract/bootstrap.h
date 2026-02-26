#pragma once

#include "refract/schema_registry.h"

#include <vector>

namespace iris::refract {

struct BootstrapResult {
  std::size_t inserted{0};
  std::size_t existing{0};
};

struct CatalogBootstrapResult {
  std::size_t inserted{0};
  std::size_t existing{0};
};

// Returns the canonical TypeDefinition set for Refract core schema.
std::vector<TypeDefinition> core_schema_definitions();

// Ensures core schema definitions exist in Referee (idempotent).
referee::Result<BootstrapResult> bootstrap_core_schema(SchemaRegistry& registry);

// Ensures core catalog objects exist in Referee (idempotent).
referee::Result<CatalogBootstrapResult> bootstrap_core_catalog(SchemaRegistry& registry,
                                                               referee::SqliteStore& store);

} // namespace iris::refract
