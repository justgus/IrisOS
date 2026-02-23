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
#include "vizier/routing.h"

#include <optional>

using namespace referee;
using namespace iris::refract;
using namespace iris::viz;
using namespace iris::vizier;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

std::optional<TypeID> find_type_id(SchemaRegistry& registry,
                                   const std::string& ns,
                                   const std::string& name) {
  auto listR = registry.list_types();
  if (!listR) return std::nullopt;
  for (const auto& summary : listR.value.value()) {
    if (summary.namespace_name == ns && summary.name == name) {
      return summary.type_id;
    }
  }
  return std::nullopt;
}

std::optional<TypeSummary> find_type_summary(SchemaRegistry& registry,
                                             const std::string& ns,
                                             const std::string& name) {
  auto listR = registry.list_types();
  if (!listR) return std::nullopt;
  for (const auto& summary : listR.value.value()) {
    if (summary.namespace_name == ns && summary.name == name) {
      return summary;
    }
  }
  return std::nullopt;
}

} // namespace

START_TEST(test_phase3_artifact_route)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TextLog log;
  log.lines = {"hello"};
  auto logR = create_text_log(registry, store, log);
  ck_assert_msg(logR, "create_text_log failed: %s", result_message(logR));

  auto recR = store.get_latest(logR.value.value());
  ck_assert_msg(recR, "get_latest failed: %s", result_message(recR));
  ck_assert_msg(recR.value->has_value(), "expected record");

  auto route = route_for_type_id(registry, recR.value->value().type);
  ck_assert_msg(route.has_value(), "expected route for TextLog");
  ck_assert_str_eq(route->concho.c_str(), "Log");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_phase3_concho_spawn)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TextLog log;
  log.lines = {"hello"};
  auto logR = create_text_log(registry, store, log);
  ck_assert_msg(logR, "create_text_log failed: %s", result_message(logR));

  auto conchoR = spawn_concho_for_artifact(registry, store, logR.value.value());
  ck_assert_msg(conchoR, "spawn_concho_for_artifact failed: %s", result_message(conchoR));
  ck_assert_msg(conchoR.value->has_value(), "expected concho spawn");

  auto conchoRecR = store.get_latest(conchoR.value->value());
  ck_assert_msg(conchoRecR, "get_latest concho failed: %s", result_message(conchoRecR));
  ck_assert_msg(conchoRecR.value->has_value(), "expected concho record");
  auto conchoType = find_type_id(registry, "Conch", "Concho");
  ck_assert_msg(conchoType.has_value(), "expected Conch::Concho type");
  ck_assert_uint_eq(conchoRecR.value->value().type.v, conchoType->v);

  auto logRecR = store.get_latest(logR.value.value());
  ck_assert_msg(logRecR, "get_latest log failed: %s", result_message(logRecR));
  ck_assert_msg(logRecR.value->has_value(), "expected log record");
  auto edgesR = store.edges_from(logRecR.value->value().ref);
  ck_assert_msg(edgesR, "edges_from failed: %s", result_message(edgesR));
  bool found = false;
  for (const auto& edge : edgesR.value.value()) {
    if (edge.name == "view" && edge.role == "concho"
        && edge.to.id.to_hex() == conchoR.value->value().to_hex()) {
      found = true;
      break;
    }
  }
  ck_assert_msg(found, "expected view->concho edge");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_phase4_demo_schema)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto demo = find_type_summary(registry, "Demo", "PropulsionSynth");
  ck_assert_msg(demo.has_value(), "expected Demo::PropulsionSynth type");
  auto summary = find_type_summary(registry, "Demo", "Summary");
  ck_assert_msg(summary.has_value(), "expected Demo::Summary type");
  auto detail = find_type_summary(registry, "Demo", "Detail");
  ck_assert_msg(detail.has_value(), "expected Demo::Detail type");

  auto demoDefR = registry.get_definition_by_type(demo->type_id);
  ck_assert_msg(demoDefR, "get_definition_by_type demo failed: %s", result_message(demoDefR));
  bool start_found = false;
  for (const auto& op : demoDefR.value->value().definition.operations) {
    if (op.name == "start") start_found = true;
  }
  ck_assert_msg(start_found, "expected start operation on Demo::PropulsionSynth");

  auto summaryDefR = registry.get_definition_by_type(summary->type_id);
  ck_assert_msg(summaryDefR, "get_definition_by_type summary failed: %s", result_message(summaryDefR));
  bool expand_found = false;
  for (const auto& op : summaryDefR.value->value().definition.operations) {
    if (op.name == "expand") {
      expand_found = true;
      ck_assert_int_eq((int)op.signature.params.size(), 1);
      ck_assert_str_eq(op.signature.params[0].name.c_str(), "level");
      ck_assert_msg(op.signature.params[0].optional, "expected optional level param");
    }
  }
  ck_assert_msg(expand_found, "expected expand operation on Demo::Summary");

  auto detailDefR = registry.get_definition_by_type(detail->type_id);
  ck_assert_msg(detailDefR, "get_definition_by_type detail failed: %s", result_message(detailDefR));
  bool has_title = false;
  bool has_level = false;
  bool has_index = false;
  for (const auto& field : detailDefR.value->value().definition.fields) {
    if (field.name == "title") has_title = true;
    if (field.name == "level") has_level = true;
    if (field.name == "index") has_index = true;
  }
  ck_assert_msg(has_title && has_level && has_index, "expected title/level/index fields on Demo::Detail");

  auto demo_payload = referee::cbor_from_json_string("{\"name\":\"Demo\"}");
  auto demoR = store.create_object(demo->type_id, demo->definition_id, demo_payload);
  ck_assert_msg(demoR, "create demo failed: %s", result_message(demoR));

  auto summary_payload = referee::cbor_from_json_string("{\"title\":\"Summary\",\"level\":0}");
  auto summaryR = store.create_object(summary->type_id, summary->definition_id, summary_payload);
  ck_assert_msg(summaryR, "create summary failed: %s", result_message(summaryR));

  auto detail_payload = referee::cbor_from_json_string("{\"title\":\"Detail\",\"level\":1,\"index\":1}");
  auto detailR = store.create_object(detail->type_id, detail->definition_id, detail_payload);
  ck_assert_msg(detailR, "create detail failed: %s", result_message(detailR));

  referee::Bytes props;
  auto edgeDemoR = store.add_edge(demoR.value->ref, summaryR.value->ref, "summary", "root", props);
  ck_assert_msg(edgeDemoR, "add_edge summary failed: %s", result_message(edgeDemoR));

  auto edgeR = store.add_edge(summaryR.value->ref, detailR.value->ref, "summarizes", "detail", props);
  ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

  Metric metric;
  metric.name = "thrust";
  metric.value = 0.7;
  auto metricR = create_metric(registry, store, metric);
  ck_assert_msg(metricR, "create_metric failed: %s", result_message(metricR));

  auto metricRecR = store.get_latest(metricR.value.value());
  ck_assert_msg(metricRecR, "get_latest metric failed: %s", result_message(metricRecR));
  ck_assert_msg(metricRecR.value->has_value(), "expected metric record");

  auto edgeR2 = store.add_edge(detailR.value->ref, metricRecR.value->value().ref,
                               "produced", "artifact", props);
  ck_assert_msg(edgeR2, "add_edge metric failed: %s", result_message(edgeR2));

  auto conchoR = spawn_concho_for_artifact(registry, store, metricR.value.value());
  ck_assert_msg(conchoR, "spawn_concho_for_artifact failed: %s", result_message(conchoR));
  ck_assert_msg(conchoR.value->has_value(), "expected concho for demo artifact");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* phase3_integration_suite(void) {
  Suite* s = suite_create("Phase3Integration");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_phase3_artifact_route);
  tcase_add_test(tc, test_phase3_concho_spawn);
  tcase_add_test(tc, test_phase4_demo_schema);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = phase3_integration_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
