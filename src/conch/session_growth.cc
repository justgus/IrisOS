#include "conch/session_growth.h"

#include "vizier/routing.h"

#include <nlohmann/json.hpp>

#include <optional>

namespace iris::conch {

namespace {

std::optional<iris::refract::TypeSummary> find_type(iris::refract::SchemaRegistry& registry,
                                                    const std::string& ns,
                                                    const std::string& name) {
  auto typesR = registry.list_types();
  if (!typesR) return std::nullopt;

  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == ns && summary.name == name) return summary;
  }
  return std::nullopt;
}

referee::Result<referee::ObjectRecord> create_concho(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    const iris::vizier::RelationshipRouteDecision& decision) {
  const char* concho_type_name = decision.route.concho == "Task" ? "TaskConcho" : "Concho";
  auto concho_type = find_type(registry, "Conch", concho_type_name);
  if (!concho_type.has_value()) {
    return referee::Result<referee::ObjectRecord>::err("Conch concho type not registered");
  }

  nlohmann::json payload;
  payload["title"] = decision.route.concho;
  if (decision.route.concho == "Task") {
    if (!decision.task_id.has_value() || !decision.task_state.has_value()) {
      return referee::Result<referee::ObjectRecord>::err("task route missing task metadata");
    }
    payload["task_id"] = decision.task_id.value();
    payload["state"] = decision.task_state.value();
    payload["task_view_id"] = decision.artifact.id.to_hex();
  }

  auto cbor = nlohmann::json::to_cbor(payload);
  return store.create_object(concho_type->type_id, concho_type->definition_id, cbor);
}

referee::Result<bool> has_observed_route(referee::SqliteStore& store,
                                         const SessionState& state,
                                         const iris::vizier::RelationshipRouteDecision& decision) {
  auto edgesR = store.edges_from(state.session, "observed", decision.relationship);
  if (!edgesR) return referee::Result<bool>::err(edgesR.error->message);

  for (const auto& edge : edgesR.value.value()) {
    if (edge.to == decision.artifact) return referee::Result<bool>::ok(true);
  }
  return referee::Result<bool>::ok(false);
}

referee::Result<void> link_concho(referee::SqliteStore& store,
                                  const SessionState& state,
                                  const iris::vizier::RelationshipRouteDecision& decision,
                                  const referee::ObjectRecord& concho) {
  nlohmann::json props;
  props["relationship"] = decision.relationship;
  props["route"] = decision.route.concho;
  auto props_cbor = nlohmann::json::to_cbor(props);

  auto viewR = store.add_edge(decision.artifact, concho.ref, "view", "concho", props_cbor);
  if (!viewR) return viewR;

  auto containsR = store.add_edge(state.session, concho.ref, "contains", "concho", props_cbor);
  if (!containsR) return containsR;

  auto observedR = store.add_edge(state.session, decision.artifact, "observed",
                                  decision.relationship, props_cbor);
  if (!observedR) return observedR;

  return referee::Result<void>::ok();
}

} // namespace

referee::Result<SessionState> create_session(iris::refract::SchemaRegistry& registry,
                                             referee::SqliteStore& store,
                                             std::string name,
                                             referee::GraphChangeCursor cursor) {
  auto session_type = find_type(registry, "Conch", "Session");
  if (!session_type.has_value()) {
    return referee::Result<SessionState>::err("Conch::Session type not registered");
  }

  nlohmann::json payload;
  payload["name"] = std::move(name);
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(session_type->type_id, session_type->definition_id, cbor);
  if (!createR) return referee::Result<SessionState>::err(createR.error->message);

  return referee::Result<SessionState>::ok(SessionState{createR.value->ref, cursor});
}

referee::Result<SessionUpdateResult> update_session_from_graph(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    SessionState& state) {
  SessionUpdateResult result;
  result.consumed_cursor = state.cursor;

  auto changesR = store.graph_changes_after(state.cursor);
  if (!changesR) return referee::Result<SessionUpdateResult>::err(changesR.error->message);

  for (const auto& change : changesR.value.value()) {
    ++result.changes_examined;
    result.consumed_cursor = change.cursor;

    auto decisionR = iris::vizier::route_for_graph_change(registry, store, change);
    if (!decisionR) {
      return referee::Result<SessionUpdateResult>::err(decisionR.error->message);
    }
    if (!decisionR.value->has_value()) continue;

    const auto& decision = decisionR.value->value();
    auto observedR = has_observed_route(store, state, decision);
    if (!observedR) return referee::Result<SessionUpdateResult>::err(observedR.error->message);
    if (observedR.value.value()) {
      ++result.conchos_reused;
      continue;
    }

    auto conchoR = create_concho(registry, store, decision);
    if (!conchoR) return referee::Result<SessionUpdateResult>::err(conchoR.error->message);

    auto linkR = link_concho(store, state, decision, conchoR.value.value());
    if (!linkR) return referee::Result<SessionUpdateResult>::err(linkR.error->message);
    ++result.conchos_created;
  }

  state.cursor = result.consumed_cursor;
  return referee::Result<SessionUpdateResult>::ok(result);
}

} // namespace iris::conch
