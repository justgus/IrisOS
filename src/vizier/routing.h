#pragma once

#include "refract/schema_registry.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdint>
#include <optional>
#include <string>

namespace iris::vizier {

struct Route {
  std::string concho;
};

struct RelationshipRouteDecision {
  referee::ObjectRef source{};
  referee::ObjectRef artifact{};
  std::string relationship;
  std::string role;
  Route route;
  std::optional<std::uint64_t> task_id{};
  std::optional<std::string> task_state{};
};

std::optional<Route> route_for_type(const iris::refract::TypeSummary& summary);
std::optional<Route> route_for_type_id(iris::refract::SchemaRegistry& registry,
                                       referee::TypeID type_id);
std::optional<RelationshipRouteDecision> route_for_relationship(
    const referee::EdgeRecord& edge,
    const iris::refract::TypeSummary& target_type);
referee::Result<std::optional<RelationshipRouteDecision>> route_for_relationship(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    const referee::EdgeRecord& edge);
referee::Result<std::optional<RelationshipRouteDecision>> route_for_graph_change(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    const referee::GraphChangeRecord& change);
referee::Result<std::optional<referee::ObjectID>> spawn_concho_for_artifact(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    referee::ObjectID artifact_id);

} // namespace iris::vizier
