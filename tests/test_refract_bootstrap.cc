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

#include <optional>
#include <string>

using namespace referee;
using namespace iris::refract;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

std::optional<TypeSummary> find_type(const std::vector<TypeSummary>& types,
                                     const std::string& ns,
                                     const std::string& name) {
  for (const auto& summary : types) {
    if (summary.namespace_name == ns && summary.name == name) return summary;
  }
  return std::nullopt;
}

bool type_has_operation(const DefinitionRecord& record, const std::string& name) {
  for (const auto& op : record.definition.operations) {
    if (op.name == name) return true;
  }
  return false;
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

START_TEST(test_bootstrap_crate_collections)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto listR = registry.list_types();
  ck_assert_msg(listR, "list_types failed: %s", result_message(listR));
  const auto& types = listR.value.value();

  auto array_type = find_type(types, "Crate", "Array");
  ck_assert_msg(array_type.has_value(), "Crate::Array missing");
  ck_assert_msg(find_type(types, "Crate", "List").has_value(), "Crate::List missing");
  ck_assert_msg(find_type(types, "Crate", "Set").has_value(), "Crate::Set missing");
  ck_assert_msg(find_type(types, "Crate", "Map").has_value(), "Crate::Map missing");
  ck_assert_msg(find_type(types, "Crate", "Tuple").has_value(), "Crate::Tuple missing");

  auto defR = registry.get_definition_by_type(array_type->type_id);
  ck_assert_msg(defR, "get_definition_by_type failed: %s", result_message(defR));
  ck_assert_msg(defR.value->has_value(), "Crate::Array definition missing");

  const auto& record = defR.value->value();
  ck_assert_msg(type_has_operation(record, "size"), "Crate::Array missing size op");
  ck_assert_msg(type_has_operation(record, "iterate"), "Crate::Array missing iterate op");
  ck_assert_msg(type_has_operation(record, "index"), "Crate::Array missing index op");
  ck_assert_msg(type_has_operation(record, "contains"), "Crate::Array missing contains op");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* refract_bootstrap_suite(void) {
  Suite* s = suite_create("RefractBootstrap");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_bootstrap_idempotent);
  tcase_add_test(tc, test_bootstrap_crate_collections);

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
