extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "refract/dispatch.h"
#include "refract/operation_registry.h"
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
  TypeDefinition def{};
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
  sig.outputs.push_back(ParameterDefinition{ "result", TypeID{0x1003ULL}, false });

  OperationDefinition op;
  op.name = "lookup";
  op.scope = OperationScope::Object;
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

START_TEST(test_schema_registry_supersedes_chain)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto defV1 = make_definition(TypeID{0xA1ULL}, "Widget", "Demo");
  defV1.version = 1;

  auto regV1 = registry.register_definition(defV1);
  ck_assert_msg(regV1, "register_definition v1 failed: %s", result_message(regV1));

  auto defV2 = make_definition(TypeID{0xA1ULL}, "Widget", "Demo");
  defV2.version = 2;
  defV2.supersedes_definition_id = regV1.value->ref.id;
  defV2.migration_hook = "migrate_widget_v1_to_v2";

  auto regV2 = registry.register_definition(defV2);
  ck_assert_msg(regV2, "register_definition v2 failed: %s", result_message(regV2));

  auto chainR = registry.list_supersedes_chain(regV2.value->ref.id);
  ck_assert_msg(chainR, "list_supersedes_chain failed: %s", result_message(chainR));
  ck_assert_int_eq((int)chainR.value->size(), 1);
  ck_assert_str_eq(chainR.value->at(0).prior.definition.name.c_str(), "Widget");
  ck_assert_msg(chainR.value->at(0).migration_hook.has_value(), "expected migration hook");
  ck_assert_str_eq(chainR.value->at(0).migration_hook->c_str(), "migrate_widget_v1_to_v2");

  auto emptyR = registry.list_supersedes_chain(regV1.value->ref.id);
  ck_assert_msg(emptyR, "list_supersedes_chain empty failed: %s", result_message(emptyR));
  ck_assert_int_eq((int)emptyR.value->size(), 0);
}
END_TEST

START_TEST(test_schema_registry_structured_metadata_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  TypeDefinition enum_def{};
  enum_def.type_id = TypeID{0xE1ULL};
  enum_def.name = "Mode";
  enum_def.namespace_name = "Demo";
  enum_def.version = 1;
  enum_def.kind = "enum";
  enum_def.enum_value_type = TypeID{0x1002ULL};
  enum_def.has_enum_value_type = true;
  enum_def.enum_values.push_back(EnumValueDefinition{ "Off", "0" });
  enum_def.enum_values.push_back(EnumValueDefinition{ "On", "1" });

  auto enum_reg = registry.register_definition(enum_def);
  ck_assert_msg(enum_reg, "register enum failed: %s", result_message(enum_reg));

  auto enum_back = registry.get_definition_by_type(enum_def.type_id);
  ck_assert_msg(enum_back, "get enum failed: %s", result_message(enum_back));
  ck_assert_msg(enum_back.value->has_value(), "enum definition missing");
  ck_assert_str_eq(enum_back.value->value().definition.kind->c_str(), "enum");
  ck_assert_msg(enum_back.value->value().definition.has_enum_value_type,
                "enum value type missing");
  ck_assert_int_eq((int)enum_back.value->value().definition.enum_values.size(), 2);

  TypeDefinition packet_def{};
  packet_def.type_id = TypeID{0xE2ULL};
  packet_def.name = "Header";
  packet_def.namespace_name = "Demo";
  packet_def.version = 1;
  packet_def.kind = "packet";
  packet_def.packet_byte_order = "be";
  packet_def.packet_fields.push_back(PacketFieldDefinition{ "magic", TypeID{0x1002ULL}, 16 });
  packet_def.packet_fields.push_back(PacketFieldDefinition{ "flags", TypeID{0x1002ULL}, 8 });

  auto packet_reg = registry.register_definition(packet_def);
  ck_assert_msg(packet_reg, "register packet failed: %s", result_message(packet_reg));

  auto packet_back = registry.get_definition_by_type(packet_def.type_id);
  ck_assert_msg(packet_back, "get packet failed: %s", result_message(packet_back));
  ck_assert_msg(packet_back.value->has_value(), "packet definition missing");
  ck_assert_str_eq(packet_back.value->value().definition.kind->c_str(), "packet");
  ck_assert_int_eq((int)packet_back.value->value().definition.packet_fields.size(), 2);
  ck_assert_str_eq(packet_back.value->value().definition.packet_fields[0].name.c_str(), "magic");
}
END_TEST

START_TEST(test_schema_registry_collection_metadata_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  TypeDefinition struct_def{};
  struct_def.type_id = TypeID{0xE3ULL};
  struct_def.name = "Point";
  struct_def.namespace_name = "Demo";
  struct_def.version = 1;
  struct_def.kind = "struct";
  struct_def.fields.push_back(FieldDefinition{ "x", TypeID{0x1008ULL}, true, std::nullopt });
  struct_def.fields.push_back(FieldDefinition{ "y", TypeID{0x1008ULL}, true, std::nullopt });

  auto struct_reg = registry.register_definition(struct_def);
  ck_assert_msg(struct_reg, "register struct failed: %s", result_message(struct_reg));

  TypeDefinition array_def{};
  array_def.type_id = TypeID{0xE4ULL};
  array_def.name = "PointArray";
  array_def.namespace_name = "Demo";
  array_def.version = 1;
  array_def.kind = "array";
  array_def.collection_kind = "array";
  array_def.collection_elements.push_back(CollectionElementDefinition{ "element", struct_def.type_id });

  auto array_reg = registry.register_definition(array_def);
  ck_assert_msg(array_reg, "register array failed: %s", result_message(array_reg));

  auto array_back = registry.get_definition_by_type(array_def.type_id);
  ck_assert_msg(array_back, "get array failed: %s", result_message(array_back));
  ck_assert_msg(array_back.value->has_value(), "array definition missing");
  ck_assert_msg(array_back.value->value().definition.collection_kind.has_value(),
                "collection kind missing");
  ck_assert_str_eq(array_back.value->value().definition.collection_kind->c_str(), "array");
  ck_assert_int_eq((int)array_back.value->value().definition.collection_elements.size(), 1);
  ck_assert_uint_eq(array_back.value->value().definition.collection_elements[0].type.v,
                    struct_def.type_id.v);
}
END_TEST

START_TEST(test_operation_registry_scope_and_inheritance)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto base = make_definition(TypeID{0xB1ULL}, "Base", "Demo");
  base.operations.clear();
  OperationDefinition base_class_op;
  base_class_op.name = "create";
  base_class_op.scope = OperationScope::Class;
  base.operations.push_back(base_class_op);

  auto derived = make_definition(TypeID{0xB2ULL}, "Derived", "Demo");
  derived.operations.clear();
  OperationDefinition derived_obj_op;
  derived_obj_op.name = "update";
  derived_obj_op.scope = OperationScope::Object;
  derived.operations.push_back(derived_obj_op);

  ck_assert_msg(registry.register_definition(base), "register base failed");
  ck_assert_msg(registry.register_definition(derived), "register derived failed");

  OperationRegistry op_registry(
      registry,
      [](TypeID type_id) -> std::vector<TypeID> {
        if (type_id.v == 0xB2ULL) return { TypeID{0xB1ULL} };
        return {};
      });

  auto objR = op_registry.list_operations(TypeID{0xB2ULL}, OperationScope::Object, true);
  ck_assert_msg(objR, "list object ops failed: %s", result_message(objR));
  ck_assert_int_eq((int)objR.value->size(), 1);
  ck_assert_str_eq(objR.value->at(0).name.c_str(), "update");

  auto classR = op_registry.list_operations(TypeID{0xB2ULL}, OperationScope::Class, true);
  ck_assert_msg(classR, "list class ops failed: %s", result_message(classR));
  ck_assert_int_eq((int)classR.value->size(), 1);
  ck_assert_str_eq(classR.value->at(0).name.c_str(), "create");
}
END_TEST

START_TEST(test_dispatch_resolution)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto base = make_definition(TypeID{0xC1ULL}, "Base", "Demo");
  base.operations.clear();
  OperationDefinition base_op;
  base_op.name = "ping";
  base_op.scope = OperationScope::Object;
  base_op.signature.params.push_back(ParameterDefinition{ "name", TypeID{0x1001ULL}, false });
  base.operations.push_back(base_op);

  auto derived = make_definition(TypeID{0xC2ULL}, "Derived", "Demo");
  derived.operations.clear();
  OperationDefinition derived_op = base_op;
  derived.operations.push_back(derived_op);

  auto overloads = make_definition(TypeID{0xD1ULL}, "Overloads", "Demo");
  overloads.operations.clear();
  OperationDefinition op_string;
  op_string.name = "op";
  op_string.scope = OperationScope::Object;
  op_string.signature.params.push_back(ParameterDefinition{ "s", TypeID{0x1001ULL}, false });
  overloads.operations.push_back(op_string);
  OperationDefinition op_num;
  op_num.name = "op";
  op_num.scope = OperationScope::Object;
  op_num.signature.params.push_back(ParameterDefinition{ "n", TypeID{0x1002ULL}, false });
  overloads.operations.push_back(op_num);

  ck_assert_msg(registry.register_definition(base), "register base failed");
  ck_assert_msg(registry.register_definition(derived), "register derived failed");
  ck_assert_msg(registry.register_definition(overloads), "register overloads failed");

  DispatchEngine engine(
      registry,
      [](TypeID type_id) -> std::vector<TypeID> {
        if (type_id.v == 0xC2ULL) return { TypeID{0xC1ULL} };
        return {};
      });

  auto overrideR = engine.resolve(
      TypeID{0xC2ULL},
      "ping",
      OperationScope::Object,
      { TypeID{0x1001ULL} },
      1,
      true);
  ck_assert_msg(overrideR, "dispatch override failed: %s", result_message(overrideR));
  ck_assert_int_eq(overrideR.value->owner_type.v, 0xC2ULL);

  auto overloadR = engine.resolve(
      TypeID{0xD1ULL},
      "op",
      OperationScope::Object,
      { TypeID{0x1002ULL} },
      1,
      false);
  ck_assert_msg(overloadR, "dispatch overload failed: %s", result_message(overloadR));
  ck_assert_str_eq(overloadR.value->operation.name.c_str(), "op");
  ck_assert_int_eq(overloadR.value->operation.signature.params[0].type.v, 0x1002ULL);

  auto ambigR = engine.resolve(
      TypeID{0xD1ULL},
      "op",
      OperationScope::Object,
      {},
      1,
      false);
  ck_assert_msg(!ambigR, "expected ambiguous dispatch failure");
  ck_assert_msg(ambigR.error.has_value(), "expected error for ambiguous dispatch");
  ck_assert_msg(ambigR.error->message.find("ambiguous") != std::string::npos,
                "expected ambiguous error, got: %s", ambigR.error->message.c_str());
}
END_TEST

Suite* refract_registry_suite(void) {
  Suite* s = suite_create("RefractRegistry");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_schema_registry_roundtrip);
  tcase_add_test(tc, test_schema_registry_supersedes_chain);
  tcase_add_test(tc, test_schema_registry_structured_metadata_roundtrip);
  tcase_add_test(tc, test_schema_registry_collection_metadata_roundtrip);
  tcase_add_test(tc, test_operation_registry_scope_and_inheritance);
  tcase_add_test(tc, test_dispatch_resolution);

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
