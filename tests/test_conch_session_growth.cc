extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "conch/session_growth.h"
#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"
#include "viz/artifacts.h"

using namespace referee;
using namespace iris::conch;
using namespace iris::refract;
using namespace iris::viz;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

ObjectRef latest_ref(SqliteStore& store, ObjectID id) {
  auto recR = store.get_latest(id);
  ck_assert_msg(recR, "get_latest failed: %s", result_message(recR));
  ck_assert_msg(recR.value->has_value(), "expected object present");
  return recR.value->value().ref;
}

std::size_t count_session_conchos(SqliteStore& store, const SessionState& state) {
  auto edgesR = store.edges_from(state.session, "contains", "concho");
  ck_assert_msg(edgesR, "edges_from failed: %s", result_message(edgesR));
  return edgesR.value->size();
}

bool has_concho_type(SqliteStore& store,
                     SchemaRegistry& registry,
                     const SessionState& state,
                     const std::string& ns,
                     const std::string& name) {
  auto typeR = registry.list_types();
  ck_assert_msg(typeR, "list_types failed: %s", result_message(typeR));

  std::optional<TypeSummary> target;
  for (const auto& summary : typeR.value.value()) {
    if (summary.namespace_name == ns && summary.name == name) target = summary;
  }
  ck_assert_msg(target.has_value(), "expected target type");

  auto edgesR = store.edges_from(state.session, "contains", "concho");
  ck_assert_msg(edgesR, "edges_from failed: %s", result_message(edgesR));
  for (const auto& edge : edgesR.value.value()) {
    auto recR = store.get_object(edge.to);
    ck_assert_msg(recR, "get_object failed: %s", result_message(recR));
    if (recR.value->has_value() && recR.value->value().type == target->type_id) return true;
  }
  return false;
}

void bootstrap_store(SqliteStore& store, SchemaRegistry& registry) {
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));
}

} // namespace

START_TEST(test_session_growth_creates_concho_for_produced_artifact)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  SchemaRegistry registry(store);
  bootstrap_store(store, registry);

  TextLog log;
  log.lines = {"ready"};
  auto logR = create_text_log(registry, store, log);
  ck_assert_msg(logR, "create_text_log failed: %s", result_message(logR));

  Panel source;
  source.title = "producer";
  auto sourceR = create_panel(registry, store, source);
  ck_assert_msg(sourceR, "create_panel failed: %s", result_message(sourceR));

  auto cursorR = store.graph_cursor();
  ck_assert_msg(cursorR, "graph_cursor failed: %s", result_message(cursorR));
  auto sessionR = create_session(registry, store, "test", cursorR.value.value());
  ck_assert_msg(sessionR, "create_session failed: %s", result_message(sessionR));
  auto session = sessionR.value.value();
  auto afterSessionR = store.graph_cursor();
  ck_assert_msg(afterSessionR, "graph_cursor failed: %s", result_message(afterSessionR));
  session.cursor = afterSessionR.value.value();

  auto edgeR = store.add_edge(latest_ref(store, sourceR.value.value()), latest_ref(store, logR.value.value()),
                              "produced", "artifact", Bytes{});
  ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

  auto updateR = update_session_from_graph(registry, store, session);
  ck_assert_msg(updateR, "update_session_from_graph failed: %s", result_message(updateR));
  ck_assert_uint_eq(updateR.value->changes_examined, 1U);
  ck_assert_uint_eq(updateR.value->conchos_created, 1U);
  ck_assert_uint_eq(updateR.value->conchos_reused, 0U);
  ck_assert_uint_eq(count_session_conchos(store, session), 1U);
  ck_assert_msg(has_concho_type(store, registry, session, "Conch", "Concho"),
                "expected Conch::Concho session link");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_session_growth_handles_progress_and_suppresses_duplicates)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  SchemaRegistry registry(store);
  bootstrap_store(store, registry);

  Metric metric;
  metric.name = "pct";
  metric.value = 0.5;
  auto metricR = create_metric(registry, store, metric);
  ck_assert_msg(metricR, "create_metric failed: %s", result_message(metricR));

  Panel source;
  source.title = "worker";
  auto sourceR = create_panel(registry, store, source);
  ck_assert_msg(sourceR, "create_panel failed: %s", result_message(sourceR));

  auto cursorR = store.graph_cursor();
  ck_assert_msg(cursorR, "graph_cursor failed: %s", result_message(cursorR));
  auto sessionR = create_session(registry, store, "progress", cursorR.value.value());
  ck_assert_msg(sessionR, "create_session failed: %s", result_message(sessionR));
  auto session = sessionR.value.value();
  auto afterSessionR = store.graph_cursor();
  ck_assert_msg(afterSessionR, "graph_cursor failed: %s", result_message(afterSessionR));
  session.cursor = afterSessionR.value.value();
  auto replay_cursor = session.cursor;

  auto edgeR = store.add_edge(latest_ref(store, sourceR.value.value()), latest_ref(store, metricR.value.value()),
                              "progress", "artifact", Bytes{});
  ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

  auto firstR = update_session_from_graph(registry, store, session);
  ck_assert_msg(firstR, "update_session_from_graph failed: %s", result_message(firstR));
  ck_assert_uint_eq(firstR.value->conchos_created, 1U);

  session.cursor = replay_cursor;
  auto secondR = update_session_from_graph(registry, store, session);
  ck_assert_msg(secondR, "update_session_from_graph failed: %s", result_message(secondR));
  ck_assert_uint_eq(secondR.value->conchos_created, 0U);
  ck_assert_uint_eq(secondR.value->conchos_reused, 1U);
  ck_assert_uint_eq(count_session_conchos(store, session), 1U);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_session_growth_creates_task_concho)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  SchemaRegistry registry(store);
  bootstrap_store(store, registry);

  TaskView task;
  task.task_id = 99;
  task.state = "Waiting";
  auto taskR = create_task_view(registry, store, task);
  ck_assert_msg(taskR, "create_task_view failed: %s", result_message(taskR));

  Panel source;
  source.title = "scheduler";
  auto sourceR = create_panel(registry, store, source);
  ck_assert_msg(sourceR, "create_panel failed: %s", result_message(sourceR));

  auto cursorR = store.graph_cursor();
  ck_assert_msg(cursorR, "graph_cursor failed: %s", result_message(cursorR));
  auto sessionR = create_session(registry, store, "tasks", cursorR.value.value());
  ck_assert_msg(sessionR, "create_session failed: %s", result_message(sessionR));
  auto session = sessionR.value.value();
  auto afterSessionR = store.graph_cursor();
  ck_assert_msg(afterSessionR, "graph_cursor failed: %s", result_message(afterSessionR));
  session.cursor = afterSessionR.value.value();

  auto edgeR = store.add_edge(latest_ref(store, sourceR.value.value()), latest_ref(store, taskR.value.value()),
                              "task_state", "task", Bytes{});
  ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

  auto updateR = update_session_from_graph(registry, store, session);
  ck_assert_msg(updateR, "update_session_from_graph failed: %s", result_message(updateR));
  ck_assert_uint_eq(updateR.value->conchos_created, 1U);
  ck_assert_msg(has_concho_type(store, registry, session, "Conch", "TaskConcho"),
                "expected Conch::TaskConcho session link");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_session_growth_ignores_unrouted_changes)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  SchemaRegistry registry(store);
  bootstrap_store(store, registry);

  Panel panel;
  panel.title = "source";
  auto sourceR = create_panel(registry, store, panel);
  ck_assert_msg(sourceR, "create_panel source failed: %s", result_message(sourceR));
  auto targetR = create_panel(registry, store, panel);
  ck_assert_msg(targetR, "create_panel target failed: %s", result_message(targetR));

  auto cursorR = store.graph_cursor();
  ck_assert_msg(cursorR, "graph_cursor failed: %s", result_message(cursorR));
  auto sessionR = create_session(registry, store, "ignore", cursorR.value.value());
  ck_assert_msg(sessionR, "create_session failed: %s", result_message(sessionR));
  auto session = sessionR.value.value();
  auto afterSessionR = store.graph_cursor();
  ck_assert_msg(afterSessionR, "graph_cursor failed: %s", result_message(afterSessionR));
  session.cursor = afterSessionR.value.value();

  auto edgeR = store.add_edge(latest_ref(store, sourceR.value.value()), latest_ref(store, targetR.value.value()),
                              "summary", "artifact", Bytes{});
  ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

  auto updateR = update_session_from_graph(registry, store, session);
  ck_assert_msg(updateR, "update_session_from_graph failed: %s", result_message(updateR));
  ck_assert_uint_eq(updateR.value->changes_examined, 1U);
  ck_assert_uint_eq(updateR.value->conchos_created, 0U);
  ck_assert_uint_eq(updateR.value->conchos_reused, 0U);
  ck_assert_uint_eq(count_session_conchos(store, session), 0U);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* conch_session_growth_suite(void) {
  Suite* s = suite_create("ConchSessionGrowth");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_session_growth_creates_concho_for_produced_artifact);
  tcase_add_test(tc, test_session_growth_handles_progress_and_suppresses_duplicates);
  tcase_add_test(tc, test_session_growth_creates_task_concho);
  tcase_add_test(tc, test_session_growth_ignores_unrouted_changes);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = conch_session_growth_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
