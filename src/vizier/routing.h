#pragma once

#include "refract/schema_registry.h"
#include "referee_sqlite/sqlite_store.h"

#include <optional>
#include <string>

namespace iris::vizier {

struct Route {
  std::string concho;
};

std::optional<Route> route_for_type(const iris::refract::TypeSummary& summary);
std::optional<Route> route_for_type_id(iris::refract::SchemaRegistry& registry,
                                       referee::TypeID type_id);
referee::Result<std::optional<referee::ObjectID>> spawn_concho_for_artifact(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    referee::ObjectID artifact_id);

} // namespace iris::vizier
