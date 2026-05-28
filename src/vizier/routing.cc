#include "vizier/routing.h"

#include "ceo/task_registry.h"
#include "viz/artifacts.h"

#include <nlohmann/json.hpp>

#include <array>

namespace iris::vizier {

static std::string display_name(const iris::refract::TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
}

static bool is_routable_relationship(const std::string& relationship) {
  return relationship == "produced"
      || relationship == "progress"
      || relationship == "diagnostic"
      || relationship == "stream";
}

static bool is_task_relationship(const std::string& relationship) {
  return relationship == "task_state";
}

static bool is_known_task_state(const std::string& state) {
  static constexpr std::array<iris::ceo::TaskState, 8> states = {
      iris::ceo::TaskState::Created,
      iris::ceo::TaskState::Running,
      iris::ceo::TaskState::Waiting,
      iris::ceo::TaskState::CancelRequested,
      iris::ceo::TaskState::Canceled,
      iris::ceo::TaskState::Completed,
      iris::ceo::TaskState::Failed,
      iris::ceo::TaskState::Killed,
  };

  for (auto known : states) {
    if (state == iris::ceo::to_string(known)) return true;
  }
  return false;
}

std::optional<Route> route_for_type(const iris::refract::TypeSummary& summary) {
  if (summary.preferred_renderer.has_value() && !summary.preferred_renderer->empty()) {
    return Route{summary.preferred_renderer.value()};
  }
  auto full = display_name(summary);
  if (full == "Viz::TextLog") return Route{"Log"};
  if (full == "Viz::Metric") return Route{"Metric"};
  if (full == "Viz::Table") return Route{"Table"};
  if (full == "Viz::Tree") return Route{"Tree"};
  if (full == "Viz::TaskView") return Route{"Task"};
  return std::nullopt;
}

std::optional<RelationshipRouteDecision> route_for_relationship(
    const referee::EdgeRecord& edge,
    const iris::refract::TypeSummary& target_type) {
  if (!is_routable_relationship(edge.name)) return std::nullopt;

  auto route = route_for_type(target_type);
  if (!route.has_value()) return std::nullopt;

  return RelationshipRouteDecision{edge.from, edge.to, edge.name, edge.role, route.value()};
}

referee::Result<std::optional<RelationshipRouteDecision>> route_for_relationship(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    const referee::EdgeRecord& edge) {
  if (!is_routable_relationship(edge.name) && !is_task_relationship(edge.name)) {
    return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
        std::optional<RelationshipRouteDecision>{});
  }

  auto artifactR = store.get_object(edge.to);
  if (!artifactR) {
    return referee::Result<std::optional<RelationshipRouteDecision>>::err(artifactR.error->message);
  }
  if (!artifactR.value->has_value()) {
    return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
        std::optional<RelationshipRouteDecision>{});
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    return referee::Result<std::optional<RelationshipRouteDecision>>::err(typesR.error->message);
  }
  for (const auto& summary : typesR.value.value()) {
    if (summary.type_id == artifactR.value->value().type) {
      if (is_task_relationship(edge.name)) {
        if (summary.type_id != iris::viz::kTypeVizTaskView) {
          return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
              std::optional<RelationshipRouteDecision>{});
        }

        nlohmann::json payload;
        try {
          payload = nlohmann::json::from_cbor(artifactR.value->value().payload_cbor);
        } catch (const nlohmann::json::exception&) {
          return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
              std::optional<RelationshipRouteDecision>{});
        }
        if (!payload.contains("task_id") || !payload["task_id"].is_number_unsigned()
            || !payload.contains("state") || !payload["state"].is_string()) {
          return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
              std::optional<RelationshipRouteDecision>{});
        }

        auto state = payload["state"].get<std::string>();
        if (!is_known_task_state(state)) {
          return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
              std::optional<RelationshipRouteDecision>{});
        }

        RelationshipRouteDecision decision{edge.from, edge.to, edge.name, edge.role, Route{"Task"}};
        decision.task_id = payload["task_id"].get<std::uint64_t>();
        decision.task_state = std::move(state);
        return referee::Result<std::optional<RelationshipRouteDecision>>::ok(decision);
      }

      return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
          route_for_relationship(edge, summary));
    }
  }

  return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
      std::optional<RelationshipRouteDecision>{});
}

referee::Result<std::optional<RelationshipRouteDecision>> route_for_graph_change(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    const referee::GraphChangeRecord& change) {
  if (change.kind != referee::GraphChangeKind::EdgeCreated || !change.edge.has_value()) {
    return referee::Result<std::optional<RelationshipRouteDecision>>::ok(
        std::optional<RelationshipRouteDecision>{});
  }

  return route_for_relationship(registry, store, change.edge.value());
}

std::optional<Route> route_for_type_id(iris::refract::SchemaRegistry& registry,
                                       referee::TypeID type_id) {
  auto listR = registry.list_types();
  if (!listR) return std::nullopt;

  for (const auto& summary : listR.value.value()) {
    if (summary.type_id == type_id) return route_for_type(summary);
  }
  return std::nullopt;
}

referee::Result<std::optional<referee::ObjectID>> spawn_concho_for_artifact(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    referee::ObjectID artifact_id) {
  auto recR = store.get_latest(artifact_id);
  if (!recR) return referee::Result<std::optional<referee::ObjectID>>::err(recR.error->message);
  if (!recR.value->has_value()) {
    return referee::Result<std::optional<referee::ObjectID>>::err("artifact not found");
  }

  auto route = route_for_type_id(registry, recR.value->value().type);
  if (!route.has_value()) {
    return referee::Result<std::optional<referee::ObjectID>>::ok(
        std::optional<referee::ObjectID>{});
  }

  auto typesR = registry.list_types();
  if (!typesR) {
    return referee::Result<std::optional<referee::ObjectID>>::err(typesR.error->message);
  }
  std::optional<iris::refract::TypeSummary> concho_type;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "Conch" && summary.name == "Concho") {
      concho_type = summary;
      break;
    }
  }
  if (!concho_type.has_value()) {
    return referee::Result<std::optional<referee::ObjectID>>::err("Conch::Concho type not registered");
  }

  nlohmann::json payload;
  payload["title"] = route->concho;
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(concho_type->type_id, concho_type->definition_id, cbor);
  if (!createR) {
    return referee::Result<std::optional<referee::ObjectID>>::err(createR.error->message);
  }

  referee::Bytes props;
  auto edgeR = store.add_edge(recR.value->value().ref, createR.value->ref, "view", "concho", props);
  if (!edgeR) {
    return referee::Result<std::optional<referee::ObjectID>>::err(edgeR.error->message);
  }

  return referee::Result<std::optional<referee::ObjectID>>::ok(createR.value->ref.id);
}

} // namespace iris::vizier
