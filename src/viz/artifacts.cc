#include "viz/artifacts.h"

#include <nlohmann/json.hpp>

namespace iris::viz {

namespace {

referee::Result<referee::ObjectID> create_with_payload(iris::refract::SchemaRegistry& registry,
                                                       referee::SqliteStore& store,
                                                       referee::TypeID type,
                                                       const nlohmann::json& payload) {
  auto defR = registry.get_definition_by_type(type);
  if (!defR) return referee::Result<referee::ObjectID>::err(defR.error->message);
  if (!defR.value->has_value()) {
    return referee::Result<referee::ObjectID>::err("definition not found");
  }
  const auto& def = defR.value->value();
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(type, def.ref.id, cbor);
  if (!createR) return referee::Result<referee::ObjectID>::err(createR.error->message);
  return referee::Result<referee::ObjectID>::ok(createR.value->ref.id);
}

} // namespace

referee::Result<referee::ObjectID> create_panel(iris::refract::SchemaRegistry& registry,
                                                referee::SqliteStore& store,
                                                const Panel& panel) {
  nlohmann::json payload;
  payload["title"] = panel.title;
  return create_with_payload(registry, store, kTypeVizPanel, payload);
}

referee::Result<referee::ObjectID> create_text_log(iris::refract::SchemaRegistry& registry,
                                                   referee::SqliteStore& store,
                                                   const TextLog& log) {
  nlohmann::json payload;
  payload["lines"] = log.lines;
  return create_with_payload(registry, store, kTypeVizTextLog, payload);
}

referee::Result<referee::ObjectID> create_metric(iris::refract::SchemaRegistry& registry,
                                                 referee::SqliteStore& store,
                                                 const Metric& metric) {
  nlohmann::json payload;
  payload["name"] = metric.name;
  payload["value"] = metric.value;
  return create_with_payload(registry, store, kTypeVizMetric, payload);
}

referee::Result<referee::ObjectID> create_table(iris::refract::SchemaRegistry& registry,
                                                referee::SqliteStore& store,
                                                const Table& table) {
  nlohmann::json payload;
  payload["columns"] = table.columns;
  payload["rows"] = table.rows;
  return create_with_payload(registry, store, kTypeVizTable, payload);
}

referee::Result<referee::ObjectID> create_tree(iris::refract::SchemaRegistry& registry,
                                               referee::SqliteStore& store,
                                               const Tree& tree) {
  nlohmann::json payload;
  payload["label"] = tree.label;
  std::vector<std::string> child_ids;
  child_ids.reserve(tree.children.size());
  for (const auto& id : tree.children) child_ids.push_back(id.to_hex());
  payload["children"] = child_ids;
  return create_with_payload(registry, store, kTypeVizTree, payload);
}

} // namespace iris::viz
