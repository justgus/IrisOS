extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"
#include "viz/artifacts.h"

using namespace referee;
using namespace iris::refract;
using namespace iris::viz;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

} // namespace

START_TEST(test_viz_artifact_create)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TextLog log;
  log.lines = {"alpha", "beta"};
  auto logR = create_text_log(registry, store, log);
  ck_assert_msg(logR, "create_text_log failed: %s", result_message(logR));

  auto recR = store.get_latest(logR.value.value());
  ck_assert_msg(recR, "get_latest failed: %s", result_message(recR));
  ck_assert_msg(recR.value->has_value(), "expected object present");
  ck_assert_uint_eq(recR.value->value().type.v, kTypeVizTextLog.v);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_viz_metric_create)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  Metric metric;
  metric.name = "load";
  metric.value = 0.42;
  auto metricR = create_metric(registry, store, metric);
  ck_assert_msg(metricR, "create_metric failed: %s", result_message(metricR));

  auto recR = store.get_latest(metricR.value.value());
  ck_assert_msg(recR, "get_latest failed: %s", result_message(recR));
  ck_assert_msg(recR.value->has_value(), "expected object present");
  ck_assert_uint_eq(recR.value->value().type.v, kTypeVizMetric.v);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_viz_schema_definitions)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto logDefR = registry.get_definition_by_type(kTypeVizTextLog);
  ck_assert_msg(logDefR, "get_definition_by_type failed: %s", result_message(logDefR));
  ck_assert_msg(logDefR.value->has_value(), "expected TextLog definition");
  ck_assert_int_eq((int)logDefR.value->value().definition.fields.size(), 1);
  ck_assert_str_eq(logDefR.value->value().definition.fields[0].name.c_str(), "lines");

  auto metricDefR = registry.get_definition_by_type(kTypeVizMetric);
  ck_assert_msg(metricDefR, "get_definition_by_type failed: %s", result_message(metricDefR));
  ck_assert_msg(metricDefR.value->has_value(), "expected Metric definition");
  ck_assert_int_eq((int)metricDefR.value->value().definition.fields.size(), 2);
  ck_assert_str_eq(metricDefR.value->value().definition.fields[0].name.c_str(), "name");
  ck_assert_str_eq(metricDefR.value->value().definition.fields[1].name.c_str(), "value");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* viz_artifact_suite(void) {
  Suite* s = suite_create("VizArtifacts");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_viz_artifact_create);
  tcase_add_test(tc, test_viz_metric_create);
  tcase_add_test(tc, test_viz_schema_definitions);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = viz_artifact_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
