#include "conch/session_growth.h"

#include "vizier/routing.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>

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

referee::Result<iris::refract::TypeSummary> require_type(
    iris::refract::SchemaRegistry& registry, const std::string& ns, const std::string& name) {
  auto type = find_type(registry, ns, name);
  if (!type.has_value()) {
    return referee::Result<iris::refract::TypeSummary>::err(
        "Conch::" + name + " type not registered");
  }
  return referee::Result<iris::refract::TypeSummary>::ok(type.value());
}

std::string object_key(const referee::ObjectRef& ref) {
  return ref.id.to_hex() + ":" + std::to_string(ref.ver.v);
}

referee::Result<referee::ObjectRef> ensure_workspace(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    referee::ObjectRef session) {
  auto edgesR = store.edges_from(session, "workspace", "workspace");
  if (!edgesR) return referee::Result<referee::ObjectRef>::err(edgesR.error->message);
  for (const auto& edge : edgesR.value.value()) {
    auto workspaceR = store.get_object(edge.to);
    if (!workspaceR) return referee::Result<referee::ObjectRef>::err(workspaceR.error->message);
    try {
      const auto payload = nlohmann::json::from_cbor(workspaceR.value->payload_cbor);
      if (payload.value("session_id", std::string{}) == session.id.to_hex()) {
        return referee::Result<referee::ObjectRef>::ok(edge.to);
      }
    } catch (const std::exception& ex) {
      return referee::Result<referee::ObjectRef>::err(
          std::string("invalid Conch::Workspace payload: ") + ex.what());
    }
  }

  auto typeR = require_type(registry, "Conch", "Workspace");
  if (!typeR) return referee::Result<referee::ObjectRef>::err(typeR.error->message);

  nlohmann::json payload;
  payload["session_id"] = session.id.to_hex();
  auto createR = store.create_object(typeR.value->type_id, typeR.value->definition_id,
                                     nlohmann::json::to_cbor(payload));
  if (!createR) return referee::Result<referee::ObjectRef>::err(createR.error->message);
  auto linkR = store.add_edge(session, createR.value->ref, "workspace", "workspace", {});
  if (!linkR) return referee::Result<referee::ObjectRef>::err(linkR.error->message);
  return referee::Result<referee::ObjectRef>::ok(createR.value->ref);
}

referee::Result<void> collect_tiles(
    referee::SqliteStore& store,
    referee::ObjectRef parent,
    referee::ObjectRef workspace,
    std::unordered_map<std::string, referee::ObjectRef>& tiles,
    std::unordered_set<std::string>& visited) {
  const auto key = object_key(parent);
  if (!visited.insert(key).second) return referee::Result<void>::ok();
  auto edgesR = store.edges_from(parent, "contains", parent == workspace ? "member" : "tile");
  if (!edgesR) return referee::Result<void>::err(edgesR.error->message);
  auto edges = edgesR.value.value();
  if (parent == workspace) {
    auto legacyEdgesR = store.edges_from(parent, "contains", "tile");
    if (!legacyEdgesR) return referee::Result<void>::err(legacyEdgesR.error->message);
    edges.insert(edges.end(), legacyEdgesR.value->begin(), legacyEdgesR.value->end());
  }
  for (const auto& edge : edges) {
    auto recR = store.get_object(edge.to);
    if (!recR) return referee::Result<void>::err(recR.error->message);
    try {
      auto payload = nlohmann::json::from_cbor(recR.value->payload_cbor);
      const auto concho_id = payload.value("concho_id", std::string{});
      if (!concho_id.empty()) tiles.emplace(concho_id, edge.to);
    } catch (const std::exception& ex) {
      return referee::Result<void>::err(std::string("invalid Conch::Tile payload: ") + ex.what());
    }
    auto childR = collect_tiles(store, edge.to, workspace, tiles, visited);
    if (!childR) return childR;
  }
  return referee::Result<void>::ok();
}

referee::Result<void> synchronize_workspace(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    referee::ObjectRef session,
    std::unordered_map<std::string, referee::ObjectRef>* desired_parent_by_concho = nullptr) {
  auto workspaceR = ensure_workspace(registry, store, session);
  if (!workspaceR) return referee::Result<void>::err(workspaceR.error->message);
  auto tileTypeR = require_type(registry, "Conch", "Tile");
  if (!tileTypeR) return referee::Result<void>::err(tileTypeR.error->message);

  std::unordered_map<std::string, referee::ObjectRef> tiles;
  std::unordered_set<std::string> visited;
  auto collectR = collect_tiles(store, workspaceR.value.value(), workspaceR.value.value(),
                                tiles, visited);
  if (!collectR) return collectR;

  auto conchosR = store.edges_from(session, "contains", "concho");
  if (!conchosR) return referee::Result<void>::err(conchosR.error->message);
  std::unordered_map<std::string, referee::ObjectRef> conchos;
  for (const auto& edge : conchosR.value.value()) conchos.emplace(edge.to.id.to_hex(), edge.to);

  for (const auto& edge : conchosR.value.value()) {
    const auto concho_id = edge.to.id.to_hex();
    if (tiles.contains(concho_id)) continue;

    auto conchoR = store.get_object(edge.to);
    if (!conchoR) return referee::Result<void>::err(conchoR.error->message);
    std::string title = "Concho";
    try {
      auto payload = nlohmann::json::from_cbor(conchoR.value->payload_cbor);
      title = payload.value("title", title);
    } catch (const std::exception& ex) {
      return referee::Result<void>::err(std::string("invalid Concho payload: ") + ex.what());
    }
    nlohmann::json tilePayload;
    tilePayload["concho_id"] = concho_id;
    tilePayload["title"] = title;
    auto createR = store.create_object(tileTypeR.value->type_id, tileTypeR.value->definition_id,
                                       nlohmann::json::to_cbor(tilePayload));
    if (!createR) return referee::Result<void>::err(createR.error->message);
    tiles.emplace(concho_id, createR.value->ref);
  }

  std::unordered_map<std::string, std::string> desired_owner_by_concho;
  std::unordered_map<std::string, referee::ObjectRef> parent_by_concho;
  for (const auto& edge : conchosR.value.value()) {
    const auto concho_id = edge.to.id.to_hex();
    referee::ObjectRef parent = workspaceR.value.value();
    auto ownersR = store.edges_to(edge.to, "contains", "concho");
    if (!ownersR) return referee::Result<void>::err(ownersR.error->message);

    for (const auto& ownerEdge : ownersR.value.value()) {
      const auto owner_id = ownerEdge.from.id.to_hex();
      if (owner_id == concho_id || !conchos.contains(owner_id) || !tiles.contains(owner_id)) {
        continue;
      }

      auto ancestor = owner_id;
      std::unordered_set<std::string> visited_owners;
      bool cycle = false;
      while (true) {
        if (!visited_owners.insert(ancestor).second) {
          cycle = true;
          break;
        }
        if (ancestor == concho_id) {
          cycle = true;
          break;
        }
        auto ownerIt = desired_owner_by_concho.find(ancestor);
        if (ownerIt == desired_owner_by_concho.end()) break;
        ancestor = ownerIt->second;
      }
      if (cycle) continue;

      desired_owner_by_concho.emplace(concho_id, owner_id);
      parent = tiles.at(owner_id);
      break;
    }
    parent_by_concho.emplace(concho_id, parent);
  }

  auto memberEdgesR = store.edges_from(workspaceR.value.value(), "contains", "member");
  if (!memberEdgesR) return referee::Result<void>::err(memberEdgesR.error->message);
  std::unordered_set<std::string> members;
  for (const auto& memberEdge : memberEdgesR.value.value()) {
    members.insert(object_key(memberEdge.to));
  }

  for (const auto& edge : conchosR.value.value()) {
    const auto concho_id = edge.to.id.to_hex();
    const auto parent = parent_by_concho.at(concho_id);
    const auto tile = tiles.at(concho_id);
    if (members.insert(object_key(tile)).second) {
      auto memberR = store.add_edge(workspaceR.value.value(), tile, "contains", "member", {});
      if (!memberR) return memberR;
    }
    if (parent == workspaceR.value.value()) continue;

    auto edgesR = store.edges_from(parent, "contains", "tile");
    if (!edgesR) return referee::Result<void>::err(edgesR.error->message);
    const auto linked = std::any_of(edgesR.value->begin(), edgesR.value->end(),
                                    [&](const referee::EdgeRecord& edge) {
                                      return edge.to == tile;
                                    });
    if (linked) continue;
    auto linkR = store.add_edge(parent, tile, "contains", "tile", {});
    if (!linkR) return linkR;
  }

  if (desired_parent_by_concho != nullptr) {
    *desired_parent_by_concho = std::move(parent_by_concho);
  }
  return referee::Result<void>::ok();
}

referee::Result<void> append_workspace_tree(
    referee::SqliteStore& store,
    referee::ObjectRef parent,
    referee::ObjectRef workspace,
    const std::unordered_map<std::string, referee::ObjectRef>& desired_parent_by_concho,
    std::size_t depth,
    std::unordered_set<std::string>& visited,
    std::ostringstream& output) {
  auto edgesR = store.edges_from(parent, "contains", parent == workspace ? "member" : "tile");
  if (!edgesR) return referee::Result<void>::err(edgesR.error->message);
  for (const auto& edge : edgesR.value.value()) {
    if (visited.contains(object_key(edge.to))) continue;
    auto recR = store.get_object(edge.to);
    if (!recR) return referee::Result<void>::err(recR.error->message);
    try {
      auto payload = nlohmann::json::from_cbor(recR.value->payload_cbor);
      const auto concho_id = payload.value("concho_id", std::string{});
      const auto desiredParent = desired_parent_by_concho.find(concho_id);
      if (desiredParent == desired_parent_by_concho.end() || desiredParent->second != parent) {
        continue;
      }
      visited.insert(object_key(edge.to));
      output << std::string(depth * 2, ' ') << "- "
             << payload.value("title", std::string("Concho")) << " ["
             << concho_id << "]\n";
    } catch (const std::exception& ex) {
      return referee::Result<void>::err(std::string("invalid Conch::Tile payload: ") + ex.what());
    }
    auto childR = append_workspace_tree(store, edge.to, workspace, desired_parent_by_concho,
                                        depth + 1, visited, output);
    if (!childR) return childR;
  }
  return referee::Result<void>::ok();
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

  auto workspaceR = ensure_workspace(registry, store, createR.value->ref);
  if (!workspaceR) return referee::Result<SessionState>::err(workspaceR.error->message);

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

  auto workspaceR = synchronize_workspace(registry, store, state.session);
  if (!workspaceR) return referee::Result<SessionUpdateResult>::err(workspaceR.error->message);

  state.cursor = result.consumed_cursor;
  return referee::Result<SessionUpdateResult>::ok(result);
}

referee::Result<std::string> workspace_tree(iris::refract::SchemaRegistry& registry,
                                            referee::SqliteStore& store,
                                            referee::ObjectRef session) {
  auto workspaceR = ensure_workspace(registry, store, session);
  if (!workspaceR) return referee::Result<std::string>::err(workspaceR.error->message);
  std::unordered_map<std::string, referee::ObjectRef> desired_parent_by_concho;
  auto syncR = synchronize_workspace(registry, store, session, &desired_parent_by_concho);
  if (!syncR) return referee::Result<std::string>::err(syncR.error->message);

  std::ostringstream output;
  output << "Workspace\n";
  std::unordered_set<std::string> visited;
  auto appendR = append_workspace_tree(store, workspaceR.value.value(),
                                       workspaceR.value.value(), desired_parent_by_concho,
                                       1, visited, output);
  if (!appendR) return referee::Result<std::string>::err(appendR.error->message);
  return referee::Result<std::string>::ok(output.str());
}

} // namespace iris::conch
