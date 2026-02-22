#include "vizier/routing.h"

#include <nlohmann/json.hpp>

namespace iris::vizier {

static std::string display_name(const iris::refract::TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
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
  return std::nullopt;
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
