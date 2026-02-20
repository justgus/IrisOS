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

using namespace referee;
using namespace iris::refract;
using namespace iris::viz;
using namespace iris::vizier;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
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

Suite* phase3_integration_suite(void) {
  Suite* s = suite_create("Phase3Integration");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_phase3_artifact_route);

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
