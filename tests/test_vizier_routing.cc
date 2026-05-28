extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "vizier/routing.h"

#include "refract/bootstrap.h"
#include "viz/artifacts.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <utility>

using iris::refract::SchemaRegistry;
using iris::refract::TypeDefinition;
using iris::refract::TypeSummary;
using iris::vizier::route_for_graph_change;
using iris::vizier::route_for_relationship;
using iris::vizier::route_for_type;
using iris::viz::TaskView;
using iris::viz::create_task_view;
using iris::refract::bootstrap_core_schema;

namespace {

template <typename T>
const char* result_message(const referee::Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

TypeDefinition make_type(referee::TypeID type_id,
                         const std::string& name,
                         const std::string& ns,
                         std::optional<std::string> preferred_renderer = std::nullopt) {
  TypeDefinition def;
  def.type_id = type_id;
  def.name = name;
  def.namespace_name = ns;
  def.preferred_renderer = std::move(preferred_renderer);
  return def;
}

} // namespace

START_TEST(test_viz_routes)
{
  TypeSummary log{referee::TypeID{1}, referee::ObjectID{}, "TextLog", "Viz", std::nullopt};
  TypeSummary metric{referee::TypeID{2}, referee::ObjectID{}, "Metric", "Viz", std::nullopt};
  TypeSummary table{referee::TypeID{3}, referee::ObjectID{}, "Table", "Viz", std::nullopt};
  TypeSummary tree{referee::TypeID{4}, referee::ObjectID{}, "Tree", "Viz", std::nullopt};

  ck_assert(route_for_type(log).has_value());
  ck_assert(route_for_type(metric).has_value());
  ck_assert(route_for_type(table).has_value());
  ck_assert(route_for_type(tree).has_value());
}
END_TEST

START_TEST(test_unknown_route)
{
  TypeSummary other{referee::TypeID{5}, referee::ObjectID{}, "Panel", "Viz", std::nullopt};
  ck_assert(!route_for_type(other).has_value());
}
END_TEST

START_TEST(test_preferred_renderer_route)
{
  TypeSummary custom{referee::TypeID{6}, referee::ObjectID{}, "Widget", "Demo", std::nullopt};
  custom.preferred_renderer = "Panel";
  auto route = route_for_type(custom);
  ck_assert(route.has_value());
  ck_assert_str_eq(route->concho.c_str(), "Panel");
}
END_TEST

START_TEST(test_relationship_routes_known_artifact_relationships)
{
  TypeSummary log{referee::TypeID{10}, referee::ObjectID{}, "TextLog", "Viz", std::nullopt};
  referee::ObjectRef source{referee::ObjectID::random(), referee::Version{1}};
  referee::ObjectRef artifact{referee::ObjectID::random(), referee::Version{1}};

  for (const auto* name : {"produced", "progress", "diagnostic", "stream"}) {
    referee::EdgeRecord edge;
    edge.from = source;
    edge.to = artifact;
    edge.name = name;
    edge.role = "artifact";

    auto decision = route_for_relationship(edge, log);
    ck_assert_msg(decision.has_value(), "expected %s relationship to route", name);
    ck_assert(decision->source == source);
    ck_assert(decision->artifact == artifact);
    ck_assert_str_eq(decision->relationship.c_str(), name);
    ck_assert_str_eq(decision->role.c_str(), "artifact");
    ck_assert_str_eq(decision->route.concho.c_str(), "Log");
  }
}
END_TEST

START_TEST(test_relationship_route_honors_preferred_renderer)
{
  TypeSummary widget{referee::TypeID{11}, referee::ObjectID{}, "Widget", "Demo", "Panel"};
  referee::EdgeRecord edge;
  edge.from = referee::ObjectRef{referee::ObjectID::random(), referee::Version{1}};
  edge.to = referee::ObjectRef{referee::ObjectID::random(), referee::Version{1}};
  edge.name = "produced";
  edge.role = "artifact";

  auto decision = route_for_relationship(edge, widget);
  ck_assert(decision.has_value());
  ck_assert_str_eq(decision->route.concho.c_str(), "Panel");
}
END_TEST

START_TEST(test_relationship_route_rejects_unknown_relationship_and_target)
{
  TypeSummary log{referee::TypeID{12}, referee::ObjectID{}, "TextLog", "Viz", std::nullopt};
  TypeSummary unknown{referee::TypeID{13}, referee::ObjectID{}, "Unknown", "Demo", std::nullopt};
  referee::EdgeRecord edge;
  edge.from = referee::ObjectRef{referee::ObjectID::random(), referee::Version{1}};
  edge.to = referee::ObjectRef{referee::ObjectID::random(), referee::Version{1}};
  edge.name = "summary";
  edge.role = "root";

  ck_assert(!route_for_relationship(edge, log).has_value());

  edge.name = "produced";
  edge.role = "artifact";
  ck_assert(!route_for_relationship(edge, unknown).has_value());
}
END_TEST

START_TEST(test_graph_change_relationship_route_uses_registry_target_type)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto sourceDefR = registry.register_definition(make_type(referee::TypeID{0x1001ULL}, "Producer", "Demo"));
  ck_assert_msg(sourceDefR, "register source type failed: %s", result_message(sourceDefR));
  auto artifactDefR = registry.register_definition(make_type(referee::TypeID{0x1002ULL}, "TextLog", "Viz"));
  ck_assert_msg(artifactDefR, "register artifact type failed: %s", result_message(artifactDefR));

  auto sourceR = store.create_object(referee::TypeID{0x1001ULL}, sourceDefR.value->ref.id,
                                     referee::Bytes{0x01});
  ck_assert_msg(sourceR, "create source failed: %s", result_message(sourceR));
  auto artifactR = store.create_object(referee::TypeID{0x1002ULL}, artifactDefR.value->ref.id,
                                       referee::Bytes{0x02});
  ck_assert_msg(artifactR, "create artifact failed: %s", result_message(artifactR));

  auto cursorR = store.graph_cursor();
  ck_assert_msg(cursorR, "graph_cursor failed: %s", result_message(cursorR));

  auto edgeR = store.add_edge(sourceR.value->ref, artifactR.value->ref, "produced", "artifact",
                              referee::Bytes{});
  ck_assert_msg(edgeR, "add edge failed: %s", result_message(edgeR));

  referee::GraphChangeFilter filter;
  filter.edge_name = "produced";
  filter.edge_role = "artifact";
  auto changesR = store.graph_changes_after(cursorR.value.value(), filter);
  ck_assert_msg(changesR, "graph_changes_after failed: %s", result_message(changesR));
  ck_assert_int_eq((int)changesR.value->size(), 1);

  auto decisionR = route_for_graph_change(registry, store, changesR.value->at(0));
  ck_assert_msg(decisionR, "route_for_graph_change failed: %s", result_message(decisionR));
  ck_assert(decisionR.value->has_value());
  ck_assert(decisionR.value->value().source == sourceR.value->ref);
  ck_assert(decisionR.value->value().artifact == artifactR.value->ref);
  ck_assert_str_eq(decisionR.value->value().relationship.c_str(), "produced");
  ck_assert_str_eq(decisionR.value->value().route.concho.c_str(), "Log");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_task_state_relationship_routes_known_task_view)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto sourceDefR = registry.register_definition(make_type(referee::TypeID{0x1101ULL}, "TaskSource", "Demo"));
  ck_assert_msg(sourceDefR, "register source type failed: %s", result_message(sourceDefR));
  auto sourceR = store.create_object(referee::TypeID{0x1101ULL}, sourceDefR.value->ref.id,
                                     referee::Bytes{0x01});
  ck_assert_msg(sourceR, "create source failed: %s", result_message(sourceR));

  TaskView task;
  task.task_id = 7;
  task.state = "Running";
  auto taskR = create_task_view(registry, store, task);
  ck_assert_msg(taskR, "create_task_view failed: %s", result_message(taskR));
  auto taskRecR = store.get_latest(taskR.value.value());
  ck_assert_msg(taskRecR, "get_latest task failed: %s", result_message(taskRecR));
  ck_assert_msg(taskRecR.value->has_value(), "expected task view");

  auto edgeR = store.add_edge(sourceR.value->ref, taskRecR.value->value().ref, "task_state", "task",
                              referee::Bytes{});
  ck_assert_msg(edgeR, "add edge failed: %s", result_message(edgeR));

  auto changesR = store.graph_changes_after(referee::GraphChangeCursor{});
  ck_assert_msg(changesR, "graph_changes_after failed: %s", result_message(changesR));
  std::optional<iris::vizier::RelationshipRouteDecision> decision;
  for (const auto& change : changesR.value.value()) {
    auto decisionR = route_for_graph_change(registry, store, change);
    ck_assert_msg(decisionR, "route_for_graph_change failed: %s", result_message(decisionR));
    if (decisionR.value->has_value()) decision = decisionR.value->value();
  }

  ck_assert_msg(decision.has_value(), "expected task route decision");
  ck_assert_str_eq(decision->route.concho.c_str(), "Task");
  ck_assert(decision->task_id.has_value());
  ck_assert_uint_eq(decision->task_id.value(), 7U);
  ck_assert(decision->task_state.has_value());
  ck_assert_str_eq(decision->task_state->c_str(), "Running");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_task_state_relationship_ignores_unknown_or_incomplete_task_metadata)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto sourceDefR = registry.register_definition(make_type(referee::TypeID{0x1102ULL}, "TaskSource", "Demo"));
  ck_assert_msg(sourceDefR, "register source type failed: %s", result_message(sourceDefR));
  auto sourceR = store.create_object(referee::TypeID{0x1102ULL}, sourceDefR.value->ref.id,
                                     referee::Bytes{0x01});
  ck_assert_msg(sourceR, "create source failed: %s", result_message(sourceR));

  auto taskDefR = registry.get_definition_by_type(iris::viz::kTypeVizTaskView);
  ck_assert_msg(taskDefR, "get_definition_by_type failed: %s", result_message(taskDefR));
  ck_assert_msg(taskDefR.value->has_value(), "expected task view definition");

  nlohmann::json unknown_payload;
  unknown_payload["task_id"] = 8;
  unknown_payload["state"] = "Paused";
  auto unknownR = store.create_object(iris::viz::kTypeVizTaskView, taskDefR.value->value().ref.id,
                                      nlohmann::json::to_cbor(unknown_payload));
  ck_assert_msg(unknownR, "create unknown task view failed: %s", result_message(unknownR));

  nlohmann::json incomplete_payload;
  incomplete_payload["task_id"] = 9;
  auto incompleteR = store.create_object(iris::viz::kTypeVizTaskView, taskDefR.value->value().ref.id,
                                         nlohmann::json::to_cbor(incomplete_payload));
  ck_assert_msg(incompleteR, "create incomplete task view failed: %s", result_message(incompleteR));

  auto unknownEdgeR = store.add_edge(sourceR.value->ref, unknownR.value->ref, "task_state", "task",
                                     referee::Bytes{});
  ck_assert_msg(unknownEdgeR, "add unknown edge failed: %s", result_message(unknownEdgeR));
  auto incompleteEdgeR = store.add_edge(sourceR.value->ref, incompleteR.value->ref, "task_state",
                                        "task", referee::Bytes{});
  ck_assert_msg(incompleteEdgeR, "add incomplete edge failed: %s", result_message(incompleteEdgeR));

  auto changesR = store.graph_changes_after(referee::GraphChangeCursor{});
  ck_assert_msg(changesR, "graph_changes_after failed: %s", result_message(changesR));
  for (const auto& change : changesR.value.value()) {
    auto decisionR = route_for_graph_change(registry, store, change);
    ck_assert_msg(decisionR, "route_for_graph_change failed: %s", result_message(decisionR));
    ck_assert_msg(!decisionR.value->has_value(), "expected incomplete task metadata to be ignored");
  }

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* vizier_routing_suite(void) {
  Suite* s = suite_create("VizierRouting");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_viz_routes);
  tcase_add_test(tc, test_unknown_route);
  tcase_add_test(tc, test_preferred_renderer_route);
  tcase_add_test(tc, test_relationship_routes_known_artifact_relationships);
  tcase_add_test(tc, test_relationship_route_honors_preferred_renderer);
  tcase_add_test(tc, test_relationship_route_rejects_unknown_relationship_and_target);
  tcase_add_test(tc, test_graph_change_relationship_route_uses_registry_target_type);
  tcase_add_test(tc, test_task_state_relationship_routes_known_task_view);
  tcase_add_test(tc, test_task_state_relationship_ignores_unknown_or_incomplete_task_metadata);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = vizier_routing_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
