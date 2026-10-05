#pragma once

#include "refract/schema_registry.h"

#include <optional>
#include <string>
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

struct CaliperCatalogUnit {
  std::string name;
  std::string symbol;
  std::string dimension;
  std::vector<std::string> systems;
  std::optional<std::string> base_symbol;
  std::optional<double> scale;
  std::optional<double> offset;
  bool override_base{false};
};

// Returns the canonical TypeDefinition set for Refract core schema.
std::vector<TypeDefinition> core_schema_definitions();

// Ensures core schema definitions exist in Referee (idempotent).
// When schemas already exist, no changes are made unless IRIS_REFRACT_SCHEMA_RECOVER is set.
referee::Result<BootstrapResult> bootstrap_core_schema(SchemaRegistry& registry);

// Ensures core catalog objects exist in Referee (idempotent).
referee::Result<CatalogBootstrapResult> bootstrap_core_catalog(SchemaRegistry& registry,
                                                               referee::SqliteStore& store);

// Returns a deterministic effective catalog. Overrides require explicit intent and the same dimension.
referee::Result<std::vector<CaliperCatalogUnit>> compose_caliper_catalog(
    const std::vector<CaliperCatalogUnit>& base,
    const std::vector<CaliperCatalogUnit>& extension);

} // namespace iris::refract
