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

#include <nlohmann/json.hpp>

using namespace referee;
using namespace iris::refract;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

} // namespace

START_TEST(test_conch_define_and_instantiate)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TypeDefinition def;
  def.type_id = TypeID{0xC0C1ULL};
  def.name = "Widget";
  def.namespace_name = "ConchTest";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "label", TypeID{0x1001ULL}, true, std::nullopt });

  auto regR = registry.register_definition(def);
  ck_assert_msg(regR, "register_definition failed: %s", result_message(regR));

  auto typesR = registry.list_types();
  ck_assert_msg(typesR, "list_types failed: %s", result_message(typesR));
  bool found = false;
  for (const auto& summary : typesR.value.value()) {
    if (summary.namespace_name == "ConchTest" && summary.name == "Widget") {
      found = true;
      ck_assert_uint_eq(summary.type_id.v, def.type_id.v);
      break;
    }
  }
  ck_assert_msg(found, "expected Widget type in registry list");

  nlohmann::json payload;
  payload["label"] = "alpha";
  auto cbor = nlohmann::json::to_cbor(payload);
  auto createR = store.create_object(def.type_id, regR.value->ref.id, cbor);
  ck_assert_msg(createR, "create_object failed: %s", result_message(createR));

  auto recR = store.get_latest(createR.value->ref.id);
  ck_assert_msg(recR, "get_latest failed: %s", result_message(recR));
  ck_assert_msg(recR.value->has_value(), "expected object record");
  ck_assert_uint_eq(recR.value->value().type.v, def.type_id.v);

  auto decoded = nlohmann::json::from_cbor(recR.value->value().payload_cbor);
  ck_assert_str_eq(decoded.value("label", "").c_str(), "alpha");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* conch_authoring_suite(void) {
  Suite* s = suite_create("ConchAuthoring");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_conch_define_and_instantiate);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = conch_authoring_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
