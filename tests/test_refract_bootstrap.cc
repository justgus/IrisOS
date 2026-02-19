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

#include <string>

using namespace referee;
using namespace iris::refract;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

} // namespace

START_TEST(test_bootstrap_idempotent)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto first = bootstrap_core_schema(registry);
  ck_assert_msg(first, "bootstrap failed: %s", result_message(first));
  ck_assert_int_gt((int)first.value->inserted, 0);

  auto second = bootstrap_core_schema(registry);
  ck_assert_msg(second, "bootstrap failed: %s", result_message(second));
  ck_assert_int_eq((int)second.value->inserted, 0);

  auto listR = registry.list_types();
  ck_assert_msg(listR, "list_types failed: %s", result_message(listR));
  ck_assert_int_gt((int)listR.value->size(), 0);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* refract_bootstrap_suite(void) {
  Suite* s = suite_create("RefractBootstrap");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_bootstrap_idempotent);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = refract_bootstrap_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
