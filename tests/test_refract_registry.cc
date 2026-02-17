extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

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

static TypeDefinition make_definition(TypeID type_id, std::string name, std::string ns) {
  TypeDefinition def;
  def.type_id = type_id;
  def.name = std::move(name);
  def.namespace_name = std::move(ns);
  def.version = 1;

  FieldDefinition field;
  field.name = "display_name";
  field.type = TypeID{0x1001ULL};
  field.required = true;
  def.fields.push_back(field);

  ParameterDefinition param;
  param.name = "id";
  param.type = TypeID{0x1002ULL};
  param.optional = false;

  SignatureDefinition sig;
  sig.params.push_back(param);
  sig.return_type = TypeID{0x1003ULL};

  OperationDefinition op;
  op.name = "lookup";
  op.signature = sig;
  def.operations.push_back(op);

  RelationshipSpec rel;
  rel.role = "parent";
  rel.cardinality = "one";
  rel.target = "Container";
  def.relationships.push_back(rel);

  return def;
}

} // namespace

START_TEST(test_schema_registry_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto defA = make_definition(TypeID{0xA1ULL}, "Widget", "Demo");
  auto defB = make_definition(TypeID{0xB2ULL}, "Gadget", "Demo");

  auto regA = registry.register_definition(defA);
  ck_assert_msg(regA, "register_definition A failed: %s", result_message(regA));

  auto regB = registry.register_definition(defB);
  ck_assert_msg(regB, "register_definition B failed: %s", result_message(regB));

  auto byId = registry.get_definition_by_id(regA.value->ref.id);
  ck_assert_msg(byId, "get_definition_by_id failed: %s", result_message(byId));
  ck_assert_msg(byId.value->has_value(), "expected definition by id");
  ck_assert_str_eq(byId.value->value().definition.name.c_str(), "Widget");

  auto byType = registry.get_definition_by_type(defB.type_id);
  ck_assert_msg(byType, "get_definition_by_type failed: %s", result_message(byType));
  ck_assert_msg(byType.value->has_value(), "expected definition by type");
  ck_assert_str_eq(byType.value->value().definition.name.c_str(), "Gadget");

  auto listR = registry.list_types();
  ck_assert_msg(listR, "list_types failed: %s", result_message(listR));
  ck_assert_int_eq((int)listR.value->size(), 2);
}
END_TEST

Suite* refract_registry_suite(void) {
  Suite* s = suite_create("RefractRegistry");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_schema_registry_roundtrip);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = refract_registry_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
