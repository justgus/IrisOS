extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "refract/bootstrap.h"
#include "refract/dispatch.h"
#include "refract/operation_registry.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <stdexcept>

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

static GenericArg type_arg(std::uint64_t type_id) {
  return GenericArg{ GenericArgKind::Type, TypeID{type_id}, {}, "", {} };
}

static GenericArg value_arg(std::uint64_t type_id, std::string value) {
  return GenericArg{ GenericArgKind::Value, {}, TypeID{type_id}, std::move(value), {} };
}

static GenericArg variadic_arg(std::vector<GenericArg> items) {
  return GenericArg{ GenericArgKind::Variadic, {}, {}, "", std::move(items) };
}

} // namespace

START_TEST(test_refract_lookups_return_typed_not_found)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto missing_id = registry.get_definition_by_id(ObjectID{});
  ck_assert_msg(!missing_id, "missing definition id unexpectedly succeeded");
  ck_assert_int_eq(static_cast<int>(missing_id.error->code),
                   static_cast<int>(ErrorCode::NotFound));

  auto missing_type = registry.get_definition_by_type(TypeID{0xE001ULL});
  ck_assert_msg(!missing_type, "missing definition type unexpectedly succeeded");
  ck_assert_int_eq(static_cast<int>(missing_type.error->code),
                   static_cast<int>(ErrorCode::NotFound));

  auto missing_latest = registry.get_latest_definition_by_type(TypeID{0xE002ULL});
  ck_assert_msg(!missing_latest, "missing latest definition unexpectedly succeeded");
  ck_assert_int_eq(static_cast<int>(missing_latest.error->code),
                   static_cast<int>(ErrorCode::NotFound));

  auto unrelated = store.create_object(TypeID{0xE003ULL}, ObjectID{}, {});
  ck_assert_msg(unrelated, "create unrelated object failed: %s", result_message(unrelated));
  auto wrong_kind = registry.get_definition_by_id(unrelated.value->ref.id);
  ck_assert_msg(!wrong_kind, "non-definition object unexpectedly decoded as a definition");
  ck_assert_msg(wrong_kind.error->code != ErrorCode::NotFound,
                "non-definition object was confused with a missing definition");
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_schema_registry_validation_and_decode_errors_are_typed)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  TypeDefinition invalid{};
  invalid.type_id = TypeID{0xE110ULL};
  auto invalidR = registry.register_definition(invalid);
  ck_assert_msg(!invalidR, "expected invalid definition error");
  ck_assert_int_eq(static_cast<int>(invalidR.error->code),
                   static_cast<int>(ErrorCode::InvalidArgument));

  auto malformed = store.create_object(kTypeDefinitionType, ObjectID{}, Bytes{});
  ck_assert_msg(malformed, "create malformed definition record failed");
  auto decoded = registry.get_definition_by_id(malformed.value->ref.id);
  ck_assert_msg(!decoded, "expected malformed definition payload error");
  ck_assert_int_eq(static_cast<int>(decoded.error->code),
                   static_cast<int>(ErrorCode::CorruptData));
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

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
  ck_assert_str_eq(byId.value.value().definition.name.c_str(), "Widget");

  auto byType = registry.get_definition_by_type(defB.type_id);
  ck_assert_msg(byType, "get_definition_by_type failed: %s", result_message(byType));
  ck_assert_str_eq(byType.value.value().definition.name.c_str(), "Gadget");

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
  ck_assert_str_eq(enum_back.value.value().definition.kind->c_str(), "enum");
  ck_assert_msg(enum_back.value.value().definition.has_enum_value_type,
                "enum value type missing");
  ck_assert_int_eq((int)enum_back.value.value().definition.enum_values.size(), 2);

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
  ck_assert_str_eq(packet_back.value.value().definition.kind->c_str(), "packet");
  ck_assert_int_eq((int)packet_back.value.value().definition.packet_fields.size(), 2);
  ck_assert_str_eq(packet_back.value.value().definition.packet_fields[0].name.c_str(), "magic");
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
  ck_assert_msg(array_back.value.value().definition.collection_kind.has_value(),
                "collection kind missing");
  ck_assert_str_eq(array_back.value.value().definition.collection_kind->c_str(), "array");
  ck_assert_int_eq((int)array_back.value.value().definition.collection_elements.size(), 1);
  ck_assert_uint_eq(array_back.value.value().definition.collection_elements[0].type.v,
                    struct_def.type_id.v);
}
END_TEST

START_TEST(test_schema_registry_inheritance_metadata_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto base = make_definition(TypeID{0xE5ULL}, "BaseWidget", "Demo");
  auto iface = make_definition(TypeID{0xE6ULL}, "Renderable", "Demo");
  ck_assert_msg(registry.register_definition(base), "register base failed");
  ck_assert_msg(registry.register_definition(iface), "register iface failed");

  auto derived = make_definition(TypeID{0xE7ULL}, "DerivedWidget", "Demo");
  derived.base_types.push_back(base.type_id);
  derived.interface_types.push_back(iface.type_id);

  auto derived_reg = registry.register_definition(derived);
  ck_assert_msg(derived_reg, "register derived failed: %s", result_message(derived_reg));

  auto byType = registry.get_definition_by_type(derived.type_id);
  ck_assert_msg(byType, "get derived failed: %s", result_message(byType));
  ck_assert_int_eq((int)byType.value.value().definition.base_types.size(), 1);
  ck_assert_uint_eq(byType.value.value().definition.base_types[0].v, base.type_id.v);
  ck_assert_int_eq((int)byType.value.value().definition.interface_types.size(), 1);
  ck_assert_uint_eq(byType.value.value().definition.interface_types[0].v, iface.type_id.v);

  auto basesR = registry.list_base_types(derived.type_id);
  ck_assert_msg(basesR, "list_base_types failed: %s", result_message(basesR));
  ck_assert_int_eq((int)basesR.value->size(), 1);
  ck_assert_uint_eq(basesR.value->at(0).v, base.type_id.v);

  auto interfacesR = registry.list_interface_types(derived.type_id);
  ck_assert_msg(interfacesR, "list_interface_types failed: %s", result_message(interfacesR));
  ck_assert_int_eq((int)interfacesR.value->size(), 1);
  ck_assert_uint_eq(interfacesR.value->at(0).v, iface.type_id.v);

  auto supertypesR = registry.list_supertypes(derived.type_id);
  ck_assert_msg(supertypesR, "list_supertypes failed: %s", result_message(supertypesR));
  ck_assert_int_eq((int)supertypesR.value->size(), 2);
}
END_TEST

START_TEST(test_schema_registry_constraint_metadata_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  TypeDefinition def{};
  def.type_id = TypeID{0xEBULL};
  def.name = "ConstrainedWidget";
  def.namespace_name = "Demo";
  def.version = 1;

  FieldDefinition field{ "title", TypeID{0x1001ULL}, false, std::nullopt };
  field.constraints.push_back(FieldConstraint{ FieldConstraintKind::NonEmpty });
  field.constraints.push_back(FieldConstraint{ FieldConstraintKind::Required });
  def.fields.push_back(field);

  RelationshipSpec rel{};
  rel.role = "child";
  rel.cardinality = "one";
  rel.target = "Demo::ChildWidget";
  rel.constraints.push_back(RelationshipConstraint{ RelationshipConstraintKind::MaxOccurs, 1 });
  rel.constraints.push_back(RelationshipConstraint{ RelationshipConstraintKind::MinOccurs, 1 });
  def.relationships.push_back(rel);

  auto reg = registry.register_definition(def);
  ck_assert_msg(reg, "register constrained definition failed: %s", result_message(reg));

  auto byType = registry.get_definition_by_type(def.type_id);
  ck_assert_msg(byType, "get constrained definition failed: %s", result_message(byType));

  const auto& stored_field = byType.value.value().definition.fields.at(0);
  ck_assert_msg(stored_field.required, "required field flag should be normalized");
  ck_assert_int_eq((int)stored_field.constraints.size(), 2);
  ck_assert_int_eq((int)stored_field.constraints[0].kind, (int)FieldConstraintKind::Required);
  ck_assert_int_eq((int)stored_field.constraints[1].kind, (int)FieldConstraintKind::NonEmpty);

  const auto& stored_rel = byType.value.value().definition.relationships.at(0);
  ck_assert_int_eq((int)stored_rel.constraints.size(), 2);
  ck_assert_int_eq((int)stored_rel.constraints[0].kind,
                   (int)RelationshipConstraintKind::MinOccurs);
  ck_assert_uint_eq(stored_rel.constraints[0].value, 1);
  ck_assert_int_eq((int)stored_rel.constraints[1].kind,
                   (int)RelationshipConstraintKind::MaxOccurs);
  ck_assert_uint_eq(stored_rel.constraints[1].value, 1);

  auto legacy = make_definition(TypeID{0xECULL}, "LegacyConstraintWidget", "Demo");
  auto legacy_reg = registry.register_definition(legacy);
  ck_assert_msg(legacy_reg, "register legacy definition failed: %s", result_message(legacy_reg));

  auto legacy_back = registry.get_definition_by_type(legacy.type_id);
  ck_assert_msg(legacy_back, "get legacy definition failed: %s", result_message(legacy_back));
  ck_assert_int_eq((int)legacy_back.value.value().definition.fields[0].constraints.size(), 1);
  ck_assert_int_eq((int)legacy_back.value.value().definition.fields[0].constraints[0].kind,
                   (int)FieldConstraintKind::Required);
  ck_assert_int_eq((int)legacy_back.value.value().definition.relationships[0].constraints.size(), 2);
  ck_assert_int_eq((int)legacy_back.value.value().definition.relationships[0].constraints[0].kind,
                   (int)RelationshipConstraintKind::MinOccurs);
  ck_assert_int_eq((int)legacy_back.value.value().definition.relationships[0].constraints[1].kind,
                   (int)RelationshipConstraintKind::MaxOccurs);
}
END_TEST

START_TEST(test_schema_registry_effects_and_documentation_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  TypeDefinition def{};
  def.type_id = TypeID{0xEFULL};
  def.name = "DocumentedWidget";
  def.namespace_name = "Demo";
  def.version = 1;
  def.documentation = DocumentationMetadata{ "A documented test type.", { "show_type Demo::DocumentedWidget" } };

  FieldDefinition field{ "title", TypeID{0x1001ULL}, true, std::nullopt };
  field.documentation = DocumentationMetadata{ "Human-facing title.", {} };
  def.fields.push_back(field);

  OperationDefinition op;
  op.name = "publish";
  op.scope = OperationScope::Object;
  OperationEffect effect;
  effect.kind = OperationEffectKind::Emits;
  effect.target = "demo.events";
  effect.description = "emits a demo event";
  op.effects.push_back(effect);
  op.documentation = DocumentationMetadata{ "Publish the widget state.", { "call publish" } };
  def.operations.push_back(op);

  RelationshipSpec rel{};
  rel.role = "owner";
  rel.cardinality = "one";
  rel.target = "Demo::Owner";
  rel.documentation = DocumentationMetadata{ "Owning object.", {} };
  def.relationships.push_back(rel);

  auto reg = registry.register_definition(def);
  ck_assert_msg(reg, "register documented definition failed: %s", result_message(reg));

  auto byType = registry.get_definition_by_type(def.type_id);
  ck_assert_msg(byType, "get documented definition failed: %s", result_message(byType));

  const auto& stored = byType.value.value().definition;
  ck_assert_msg(stored.documentation.has_value(), "type documentation missing");
  ck_assert_str_eq(stored.documentation->summary->c_str(), "A documented test type.");
  ck_assert_int_eq((int)stored.documentation->examples.size(), 1);

  ck_assert_msg(stored.fields[0].documentation.has_value(), "field documentation missing");
  ck_assert_str_eq(stored.fields[0].documentation->summary->c_str(), "Human-facing title.");

  ck_assert_int_eq((int)stored.operations[0].effects.size(), 1);
  ck_assert_int_eq((int)stored.operations[0].effects[0].kind, (int)OperationEffectKind::Emits);
  ck_assert_str_eq(stored.operations[0].effects[0].target.c_str(), "demo.events");
  ck_assert_msg(stored.operations[0].documentation.has_value(), "operation documentation missing");
  ck_assert_str_eq(stored.operations[0].documentation->summary->c_str(), "Publish the widget state.");

  ck_assert_msg(stored.relationships[0].documentation.has_value(), "relationship documentation missing");
  ck_assert_str_eq(stored.relationships[0].documentation->summary->c_str(), "Owning object.");

  auto listR = registry.list_types();
  ck_assert_msg(listR, "list_types failed: %s", result_message(listR));
  bool found_summary = false;
  for (const auto& summary : listR.value.value()) {
    if (summary.type_id == def.type_id) {
      ck_assert_msg(summary.documentation.has_value(), "summary documentation missing");
      ck_assert_str_eq(summary.documentation->summary->c_str(), "A documented test type.");
      found_summary = true;
    }
  }
  ck_assert_msg(found_summary, "documented summary missing");

  OperationRegistry op_registry(registry);
  auto opsR = op_registry.list_operations(def.type_id, OperationScope::Object, false);
  ck_assert_msg(opsR, "list operations failed: %s", result_message(opsR));
  ck_assert_int_eq((int)opsR.value->size(), 1);
  ck_assert_int_eq((int)opsR.value->at(0).effects.size(), 1);
  ck_assert_msg(opsR.value->at(0).documentation.has_value(), "listed op documentation missing");
}
END_TEST

START_TEST(test_schema_registry_rejects_invalid_relationship_constraint_bounds)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  TypeDefinition def{};
  def.type_id = TypeID{0xEDULL};
  def.name = "InvalidRelationshipWidget";
  def.namespace_name = "Demo";
  def.version = 1;

  RelationshipSpec rel{};
  rel.role = "child";
  rel.cardinality = "many";
  rel.target = "Demo::ChildWidget";
  rel.constraints.push_back(RelationshipConstraint{ RelationshipConstraintKind::MinOccurs, 2 });
  rel.constraints.push_back(RelationshipConstraint{ RelationshipConstraintKind::MaxOccurs, 1 });
  def.relationships.push_back(rel);

  auto reg = registry.register_definition(def);
  ck_assert_msg(!reg, "expected invalid relationship constraints to fail");
  ck_assert_msg(reg.error.has_value(), "expected invalid relationship error");
  ck_assert_msg(reg.error->message.find("min_occurs exceeds max_occurs") != std::string::npos,
                "unexpected error: %s", reg.error->message.c_str());
}
END_TEST

START_TEST(test_schema_registry_legacy_relationship_inheritance_fallback)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto base = make_definition(TypeID{0xE8ULL}, "LegacyBase", "Demo");
  auto iface = make_definition(TypeID{0xE9ULL}, "LegacyIface", "Demo");
  ck_assert_msg(registry.register_definition(base), "register base failed");
  ck_assert_msg(registry.register_definition(iface), "register iface failed");

  auto derived = make_definition(TypeID{0xEAULL}, "LegacyDerived", "Demo");
  derived.relationships.clear();
  derived.relationships.push_back(RelationshipSpec{ "base", "one", "Demo::LegacyBase" });
  derived.relationships.push_back(RelationshipSpec{ "implements", "many", "Demo::LegacyIface" });
  auto reg = registry.register_definition(derived);
  ck_assert_msg(reg, "register derived failed: %s", result_message(reg));

  auto basesR = registry.list_base_types(derived.type_id);
  ck_assert_msg(basesR, "legacy base fallback failed: %s", result_message(basesR));
  ck_assert_int_eq((int)basesR.value->size(), 1);
  ck_assert_uint_eq(basesR.value->at(0).v, base.type_id.v);

  auto interfacesR = registry.list_interface_types(derived.type_id);
  ck_assert_msg(interfacesR, "legacy interface fallback failed: %s", result_message(interfacesR));
  ck_assert_int_eq((int)interfacesR.value->size(), 1);
  ck_assert_uint_eq(interfacesR.value->at(0).v, iface.type_id.v);
}
END_TEST

START_TEST(test_generic_instance_type_id_deterministic)
{
  GenericInstance instance{};
  instance.base_type = TypeID{0x4352415400000001ULL};

  GenericArg type_arg;
  type_arg.kind = GenericArgKind::Type;
  type_arg.type_id = TypeID{0x1001ULL};
  instance.args.push_back(type_arg);

  GenericArg value_arg;
  value_arg.kind = GenericArgKind::Value;
  value_arg.value_type = TypeID{0x1002ULL};
  value_arg.value_json = "4";
  instance.args.push_back(value_arg);

  auto keyR = encode_generic_instance_key(instance);
  ck_assert_msg(keyR, "encode key failed: %s", result_message(keyR));
  auto typeR = derive_generic_type_id(instance);
  ck_assert_msg(typeR, "derive type id failed: %s", result_message(typeR));

  auto againR = derive_generic_type_id(instance);
  ck_assert_msg(againR, "derive type id again failed: %s", result_message(againR));
  ck_assert_uint_eq(typeR.value->v, againR.value->v);

  GenericInstance reordered = instance;
  std::swap(reordered.args[0], reordered.args[1]);
  auto diffR = derive_generic_type_id(reordered);
  ck_assert_msg(diffR, "derive type id reorder failed: %s", result_message(diffR));
  ck_assert_uint_ne(typeR.value->v, diffR.value->v);
}
END_TEST

START_TEST(test_generic_instance_registry_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  GenericInstance instance{};
  instance.base_type = TypeID{0x4352415400000001ULL};
  instance.display = "Crate::Array<String>";
  instance.args.push_back(GenericArg{ GenericArgKind::Type, TypeID{0x1001ULL}, {}, "", {} });

  GenericRegistry generics(registry, store);
  auto regR = generics.register_instance(instance);
  ck_assert_msg(regR, "register instance failed: %s", result_message(regR));

  auto lookupR = generics.get_instance_by_type(regR.value->instance.instance_type);
  ck_assert_msg(lookupR, "lookup instance failed: %s", result_message(lookupR));
  ck_assert_uint_eq(lookupR.value.value().instance.instance_type.v,
                    regR.value->instance.instance_type.v);

  auto missing = generics.get_instance_by_type(TypeID{0xE004ULL});
  ck_assert_msg(!missing, "missing generic instance unexpectedly succeeded");
  ck_assert_int_eq(static_cast<int>(missing.error->code),
                   static_cast<int>(ErrorCode::NotFound));
}
END_TEST

START_TEST(test_generic_pack_contracts_validate_before_persistence)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));
  GenericRegistry generics(registry, store);

  std::vector<GenericInstance> valid;
  GenericInstance tuple_empty{};
  tuple_empty.base_type = TypeID{0x4352415400000005ULL};
  tuple_empty.args.push_back(variadic_arg({}));
  valid.push_back(tuple_empty);

  GenericInstance tuple_zero_type{};
  tuple_zero_type.base_type = TypeID{0x4352415400000005ULL};
  tuple_zero_type.args.push_back(variadic_arg({type_arg(0)}));
  valid.push_back(tuple_zero_type);

  GenericInstance tuple_one{};
  tuple_one.base_type = TypeID{0x4352415400000005ULL};
  tuple_one.args.push_back(variadic_arg({type_arg(0x1001ULL)}));
  valid.push_back(tuple_one);

  GenericInstance tuple_many{};
  tuple_many.base_type = TypeID{0x4352415400000005ULL};
  tuple_many.args.push_back(variadic_arg({type_arg(0x1001ULL), type_arg(0x1002ULL)}));
  valid.push_back(tuple_many);

  GenericInstance tensor_one{};
  tensor_one.base_type = TypeID{0x4153545200000005ULL};
  tensor_one.args.push_back(type_arg(0x1008ULL));
  tensor_one.args.push_back(variadic_arg({value_arg(0x1002ULL, "3")}));
  valid.push_back(tensor_one);

  GenericInstance tensor_many{};
  tensor_many.base_type = TypeID{0x4153545200000005ULL};
  tensor_many.args.push_back(type_arg(0x1008ULL));
  tensor_many.args.push_back(variadic_arg({value_arg(0x1002ULL, "2"),
                                           value_arg(0x1002ULL, "4")}));
  valid.push_back(tensor_many);

  for (const auto& instance : valid) {
    auto registered = generics.register_instance(instance);
    ck_assert_msg(registered, "valid pack rejected: %s", result_message(registered));
    auto loaded = generics.get_instance_by_type(registered.value->instance.instance_type);
    ck_assert_msg(loaded, "valid pack did not load: %s", result_message(loaded));
  }

  ScopedTypeRegistry scoped(ScopedTypeRegistry::Scope::Operation, generics);
  auto tuple_resolved = scoped.resolve_or_register(
      tuple_many, ScopedTypeRegistry::PromotionPolicy::LocalOnly);
  ck_assert_msg(tuple_resolved, "valid Tuple pack did not resolve: %s", result_message(tuple_resolved));
  auto tensor_resolved = scoped.resolve_or_register(
      tensor_many, ScopedTypeRegistry::PromotionPolicy::LocalOnly);
  ck_assert_msg(tensor_resolved, "valid Tensor pack did not resolve: %s", result_message(tensor_resolved));

  std::vector<GenericInstance> invalid;
  GenericInstance tuple_missing{};
  tuple_missing.base_type = TypeID{0x4352415400000005ULL};
  invalid.push_back(tuple_missing);
  GenericInstance tuple_not_pack{};
  tuple_not_pack.base_type = TypeID{0x4352415400000005ULL};
  tuple_not_pack.args.push_back(type_arg(0x1001ULL));
  invalid.push_back(tuple_not_pack);
  GenericInstance tuple_nested = tuple_empty;
  tuple_nested.args[0].items.push_back(variadic_arg({type_arg(0x1001ULL)}));
  invalid.push_back(tuple_nested);
  GenericInstance tuple_wrong_item = tuple_empty;
  tuple_wrong_item.args[0].items.push_back(value_arg(0x1002ULL, "1"));
  invalid.push_back(tuple_wrong_item);
  GenericInstance tuple_extra = tuple_one;
  tuple_extra.args.push_back(type_arg(0x1002ULL));
  invalid.push_back(tuple_extra);

  GenericInstance tensor_missing{};
  tensor_missing.base_type = TypeID{0x4153545200000005ULL};
  invalid.push_back(tensor_missing);
  GenericInstance tensor_no_pack{};
  tensor_no_pack.base_type = TypeID{0x4153545200000005ULL};
  tensor_no_pack.args.push_back(type_arg(0x1008ULL));
  invalid.push_back(tensor_no_pack);
  GenericInstance tensor_empty = tensor_one;
  tensor_empty.args[1] = variadic_arg({});
  invalid.push_back(tensor_empty);
  GenericInstance tensor_wrong_element = tensor_one;
  tensor_wrong_element.args[0] = value_arg(0x1002ULL, "1");
  invalid.push_back(tensor_wrong_element);
  GenericInstance tensor_wrong_extent_kind = tensor_one;
  tensor_wrong_extent_kind.args[1] = variadic_arg({type_arg(0x1002ULL)});
  invalid.push_back(tensor_wrong_extent_kind);
  GenericInstance tensor_wrong_extent_type = tensor_one;
  tensor_wrong_extent_type.args[1] = variadic_arg({value_arg(0x1001ULL, "1")});
  invalid.push_back(tensor_wrong_extent_type);
  GenericInstance tensor_zero = tensor_one;
  tensor_zero.args[1] = variadic_arg({value_arg(0x1002ULL, "0")});
  invalid.push_back(tensor_zero);
  GenericInstance tensor_negative = tensor_one;
  tensor_negative.args[1] = variadic_arg({value_arg(0x1002ULL, "-1")});
  invalid.push_back(tensor_negative);
  GenericInstance tensor_fraction = tensor_one;
  tensor_fraction.args[1] = variadic_arg({value_arg(0x1002ULL, "1.5")});
  invalid.push_back(tensor_fraction);
  GenericInstance tensor_invalid_json = tensor_one;
  tensor_invalid_json.args[1] = variadic_arg({value_arg(0x1002ULL, "one")});
  invalid.push_back(tensor_invalid_json);
  GenericInstance tensor_overflow = tensor_one;
  tensor_overflow.args[1] = variadic_arg({value_arg(0x1002ULL, "18446744073709551616")});
  invalid.push_back(tensor_overflow);
  GenericInstance tensor_nested = tensor_one;
  tensor_nested.args[1] = variadic_arg({variadic_arg({value_arg(0x1002ULL, "1")})});
  invalid.push_back(tensor_nested);
  GenericInstance tensor_extra = tensor_one;
  tensor_extra.args.push_back(type_arg(0x1001ULL));
  invalid.push_back(tensor_extra);

  for (const auto& instance : invalid) {
    auto rejected = generics.register_instance(instance);
    ck_assert_msg(!rejected, "malformed pack unexpectedly registered");
    ck_assert_int_eq(static_cast<int>(rejected.error->code),
                     static_cast<int>(ErrorCode::InvalidArgument));
  }

  auto rejected_resolution = scoped.resolve_or_register(
      tensor_zero, ScopedTypeRegistry::PromotionPolicy::LocalOnly);
  ck_assert_msg(!rejected_resolution, "malformed pack unexpectedly resolved");
  ck_assert_int_eq(static_cast<int>(rejected_resolution.error->code),
                   static_cast<int>(ErrorCode::InvalidArgument));

  GenericInstance fixed_arity{};
  fixed_arity.base_type = TypeID{0x4352415400000001ULL};
  fixed_arity.args.push_back(type_arg(0x1001ULL));
  auto fixed_registered = generics.register_instance(fixed_arity);
  ck_assert_msg(fixed_registered, "fixed-arity Array compatibility failed: %s",
                result_message(fixed_registered));

  auto records = store.list_by_type(kTypeGenericInstanceType);
  ck_assert_msg(records, "list generic instances failed: %s", result_message(records));
  ck_assert_uint_eq(records.value->size(), valid.size() + 1);
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_generic_pack_contracts_reject_corrupt_stored_instance)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));
  auto defR = registry.get_definition_by_type(kTypeGenericInstanceType);
  ck_assert_msg(defR, "generic instance definition missing: %s", result_message(defR));

  GenericRegistry generics(registry, store);
  std::vector<nlohmann::json> malformed_args;
  malformed_args.push_back(nlohmann::json::array({
      {{"kind", "variadic"}, {"items", nlohmann::json::array({
          {{"kind", "value"}, {"value_type_id", 0x1002ULL}, {"value_json", "1"}}
      })}}
  }));
  malformed_args.push_back(nlohmann::json::array({
      {{"kind", "variadic"}, {"items", nlohmann::json::array({
          {{"kind", "bogus"}, {"type_id", 0x1001ULL}}
      })}}
  }));
  malformed_args.push_back(nlohmann::json::array({
      {{"kind", "variadic"}, {"items", nlohmann::json::array({nlohmann::json::object()})}}
  }));
  malformed_args.push_back(nlohmann::json::array({
      {{"kind", "variadic"}, {"items", nlohmann::json::array({{{"kind", "type"}}})}}
  }));

  for (std::size_t i = 0; i < malformed_args.size(); ++i) {
    nlohmann::json payload;
    payload["base_type_id"] = 0x4352415400000005ULL;
    payload["instance_type_id"] = 0xEE001ULL + i;
    payload["args"] = nlohmann::json::binary(nlohmann::json::to_cbor(malformed_args[i]));
    auto created = store.create_object(kTypeGenericInstanceType, defR.value->ref.id,
                                       nlohmann::json::to_cbor(payload));
    ck_assert_msg(created, "create malformed generic record failed: %s", result_message(created));

    auto loaded = generics.get_instance_by_type(TypeID{0xEE001ULL + i});
    ck_assert_msg(!loaded, "malformed stored pack unexpectedly loaded");
    ck_assert_int_eq(static_cast<int>(loaded.error->code),
                     static_cast<int>(ErrorCode::CorruptData));
  }
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_astra_shape_contracts_validate_and_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));
  GenericRegistry generics(registry, store);

  const std::string max_u64 = std::to_string(std::numeric_limits<std::uint64_t>::max());
  std::vector<GenericInstance> valid;
  GenericInstance vector_one{};
  vector_one.base_type = TypeID{0x4153545200000003ULL};
  vector_one.args = {type_arg(0x1008ULL), value_arg(0x1002ULL, "1")};
  valid.push_back(vector_one);

  GenericInstance vector_max{};
  vector_max.base_type = TypeID{0x4153545200000003ULL};
  vector_max.args = {type_arg(0x1008ULL), value_arg(0x1002ULL, max_u64)};
  valid.push_back(vector_max);

  GenericInstance matrix_max{};
  matrix_max.base_type = TypeID{0x4153545200000004ULL};
  matrix_max.args = {type_arg(0x1008ULL), value_arg(0x1002ULL, "1"),
                     value_arg(0x1002ULL, max_u64)};
  valid.push_back(matrix_max);

  GenericInstance tensor_one{};
  tensor_one.base_type = TypeID{0x4153545200000005ULL};
  tensor_one.args = {type_arg(0x1008ULL),
                     variadic_arg({value_arg(0x1002ULL, max_u64)})};
  valid.push_back(tensor_one);

  GenericInstance tensor_many{};
  tensor_many.base_type = TypeID{0x4153545200000005ULL};
  tensor_many.args = {type_arg(0x1008ULL),
                      variadic_arg({value_arg(0x1002ULL, "2"),
                                    value_arg(0x1002ULL, "3")})};
  valid.push_back(tensor_many);

  for (const auto& instance : valid) {
    auto registered = generics.register_instance(instance);
    ck_assert_msg(registered, "valid Astra shape rejected: %s", result_message(registered));
    auto loaded = generics.get_instance_by_type(registered.value->instance.instance_type);
    ck_assert_msg(loaded, "valid Astra shape did not load: %s", result_message(loaded));
    ck_assert_uint_eq(loaded.value->instance.instance_type.v,
                      registered.value->instance.instance_type.v);
  }

  std::vector<GenericInstance> invalid;
  GenericInstance vector_missing = vector_one;
  vector_missing.args.pop_back();
  invalid.push_back(vector_missing);
  GenericInstance vector_extra = vector_one;
  vector_extra.args.push_back(value_arg(0x1002ULL, "1"));
  invalid.push_back(vector_extra);
  GenericInstance vector_zero = vector_one;
  vector_zero.args[1] = value_arg(0x1002ULL, "0");
  invalid.push_back(vector_zero);
  GenericInstance vector_negative = vector_one;
  vector_negative.args[1] = value_arg(0x1002ULL, "-1");
  invalid.push_back(vector_negative);
  GenericInstance vector_wrong_kind = vector_one;
  vector_wrong_kind.args[1] = type_arg(0x1002ULL);
  invalid.push_back(vector_wrong_kind);
  GenericInstance vector_wrong_value_type = vector_one;
  vector_wrong_value_type.args[1] = value_arg(0x1001ULL, "1");
  invalid.push_back(vector_wrong_value_type);

  GenericInstance matrix_missing = matrix_max;
  matrix_missing.args.pop_back();
  invalid.push_back(matrix_missing);
  GenericInstance matrix_extra = matrix_max;
  matrix_extra.args.push_back(value_arg(0x1002ULL, "1"));
  invalid.push_back(matrix_extra);
  GenericInstance matrix_overflow = matrix_max;
  matrix_overflow.args[1] = value_arg(0x1002ULL, "2");
  invalid.push_back(matrix_overflow);

  GenericInstance tensor_overflow = tensor_many;
  tensor_overflow.args[1] = variadic_arg({value_arg(0x1002ULL, max_u64),
                                          value_arg(0x1002ULL, "2")});
  invalid.push_back(tensor_overflow);

  for (const auto& instance : invalid) {
    auto derived = derive_generic_type_id(instance);
    ck_assert_msg(!derived, "invalid Astra shape unexpectedly derived a type ID");
    ck_assert_int_eq(static_cast<int>(derived.error->code),
                     static_cast<int>(ErrorCode::InvalidArgument));
    auto rejected = generics.register_instance(instance);
    ck_assert_msg(!rejected, "invalid Astra shape unexpectedly registered");
    ck_assert_int_eq(static_cast<int>(rejected.error->code),
                     static_cast<int>(ErrorCode::InvalidArgument));
  }

  auto records = store.list_by_type(kTypeGenericInstanceType);
  ck_assert_msg(records, "list generic instances failed: %s", result_message(records));
  ck_assert_uint_eq(records.value->size(), valid.size());

  auto defR = registry.get_definition_by_type(kTypeGenericInstanceType);
  ck_assert_msg(defR, "generic instance definition missing: %s", result_message(defR));
  nlohmann::json malformed_args = nlohmann::json::array({
      {{"kind", "type"}, {"type_id", 0x1008ULL}},
      {{"kind", "value"}, {"value_type_id", 0x1002ULL}, {"value_json", "0"}}
  });
  nlohmann::json payload;
  payload["base_type_id"] = 0x4153545200000003ULL;
  payload["instance_type_id"] = 0xEE101ULL;
  payload["args"] = nlohmann::json::binary(nlohmann::json::to_cbor(malformed_args));
  auto created = store.create_object(kTypeGenericInstanceType, defR.value->ref.id,
                                     nlohmann::json::to_cbor(payload));
  ck_assert_msg(created, "create malformed Astra shape failed: %s", result_message(created));
  auto malformed = generics.get_instance_by_type(TypeID{0xEE101ULL});
  ck_assert_msg(!malformed, "malformed persisted Astra shape unexpectedly loaded");
  ck_assert_int_eq(static_cast<int>(malformed.error->code),
                   static_cast<int>(ErrorCode::CorruptData));

  nlohmann::json malformed_tensor_args = nlohmann::json::array({
      {{"kind", "type"}, {"type_id", 0x1008ULL}},
      {{"kind", "variadic"}, {"items", nlohmann::json::array({
          {{"kind", "value"}, {"value_type_id", 0x1002ULL},
           {"value_json", max_u64}},
          {{"kind", "value"}, {"value_type_id", 0x1002ULL},
           {"value_json", "2"}}
      })}}
  });
  nlohmann::json malformed_tensor_payload;
  malformed_tensor_payload["base_type_id"] = 0x4153545200000005ULL;
  malformed_tensor_payload["instance_type_id"] = 0xEE102ULL;
  malformed_tensor_payload["args"] =
      nlohmann::json::binary(nlohmann::json::to_cbor(malformed_tensor_args));
  auto tensor_created = store.create_object(kTypeGenericInstanceType, defR.value->ref.id,
                                            nlohmann::json::to_cbor(malformed_tensor_payload));
  ck_assert_msg(tensor_created, "create malformed Tensor shape failed: %s",
                result_message(tensor_created));
  auto malformed_tensor = generics.get_instance_by_type(TypeID{0xEE102ULL});
  ck_assert_msg(!malformed_tensor, "overflowing persisted Tensor shape unexpectedly loaded");
  ck_assert_int_eq(static_cast<int>(malformed_tensor.error->code),
                   static_cast<int>(ErrorCode::CorruptData));

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_resolve_or_register_propagates_corrupt_registry_data)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto instance_type = registry.get_definition_by_type(kTypeGenericInstanceType);
  ck_assert_msg(instance_type, "generic instance type missing: %s", result_message(instance_type));
  auto corrupt = store.create_object(kTypeGenericInstanceType, instance_type.value->ref.id,
                                     Bytes{0xFF});
  ck_assert_msg(corrupt, "create corrupt generic record failed: %s", result_message(corrupt));

  GenericRegistry generics(registry, store);
  ScopedTypeRegistry scoped(ScopedTypeRegistry::Scope::Operation, generics);
  GenericInstance instance{};
  instance.base_type = TypeID{0x4352415400000001ULL};
  instance.args.push_back(GenericArg{ GenericArgKind::Type, TypeID{0xE005ULL}, {}, "", {} });

  auto resolved = scoped.resolve_or_register(
      instance, ScopedTypeRegistry::PromotionPolicy::LocalOnly);
  ck_assert_msg(!resolved, "corrupt registry data unexpectedly registered an instance");
  ck_assert_int_eq(static_cast<int>(resolved.error->code),
                   static_cast<int>(ErrorCode::CorruptData));

  auto records = store.list_by_type(kTypeGenericInstanceType);
  ck_assert_msg(records, "list generic instance records failed: %s", result_message(records));
  ck_assert_uint_eq(records.value->size(), 1U);
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_scoped_type_registry_promotion)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  GenericRegistry generics(registry, store);
  ScopedTypeRegistry root(ScopedTypeRegistry::Scope::Global, generics);
  ScopedTypeRegistry app(ScopedTypeRegistry::Scope::Application, generics, &root);
  ScopedTypeRegistry op(ScopedTypeRegistry::Scope::Operation, generics, &app);

  GenericInstance instance{};
  instance.base_type = TypeID{0x4352415400000001ULL};
  instance.args.push_back(GenericArg{ GenericArgKind::Type, TypeID{0x1001ULL}, {}, "", {} });

  auto regR = op.resolve_or_register(instance, ScopedTypeRegistry::PromotionPolicy::Parent);
  ck_assert_msg(regR, "register instance failed: %s", result_message(regR));

  auto type_id = regR.value->instance.instance_type;
  ck_assert_msg(op.find_local(type_id).has_value(), "op scope missing cache");
  ck_assert_msg(app.find_local(type_id).has_value(), "app scope missing cache");
  ck_assert_msg(!root.find_local(type_id).has_value(), "root scope should not be promoted");

  GenericInstance instance2{};
  instance2.base_type = TypeID{0x4352415400000001ULL};
  instance2.args.push_back(GenericArg{ GenericArgKind::Type, TypeID{0x1002ULL}, {}, "", {} });

  auto regR2 = op.resolve_or_register(instance2, ScopedTypeRegistry::PromotionPolicy::Root);
  ck_assert_msg(regR2, "register instance2 failed: %s", result_message(regR2));

  auto type_id2 = regR2.value->instance.instance_type;
  ck_assert_msg(root.find_local(type_id2).has_value(), "root scope missing promotion");
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
  derived.base_types.push_back(base.type_id);
  OperationDefinition derived_obj_op;
  derived_obj_op.name = "update";
  derived_obj_op.scope = OperationScope::Object;
  derived.operations.push_back(derived_obj_op);

  ck_assert_msg(registry.register_definition(base), "register base failed");
  ck_assert_msg(registry.register_definition(derived), "register derived failed");

  OperationRegistry op_registry(registry);

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
  OperationDefinition inherited_op;
  inherited_op.name = "pong";
  inherited_op.scope = OperationScope::Object;
  inherited_op.signature.params.push_back(ParameterDefinition{ "count", TypeID{0x1002ULL}, false });
  base.operations.push_back(inherited_op);

  auto derived = make_definition(TypeID{0xC2ULL}, "Derived", "Demo");
  derived.operations.clear();
  derived.base_types.push_back(base.type_id);
  OperationDefinition derived_op = base_op;
  derived.operations.push_back(derived_op);

  auto value_base = make_definition(TypeID{0xC3ULL}, "ValueBase", "Demo");
  value_base.operations.clear();

  auto value_derived = make_definition(TypeID{0xC4ULL}, "ValueDerived", "Demo");
  value_derived.operations.clear();
  value_derived.base_types.push_back(value_base.type_id);

  auto receiver = make_definition(TypeID{0xC5ULL}, "Receiver", "Demo");
  receiver.operations.clear();
  OperationDefinition accept_op;
  accept_op.name = "accept";
  accept_op.scope = OperationScope::Object;
  accept_op.signature.params.push_back(ParameterDefinition{ "value", value_base.type_id, false });
  receiver.operations.push_back(accept_op);

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
  ck_assert_msg(registry.register_definition(value_base), "register value_base failed");
  ck_assert_msg(registry.register_definition(value_derived), "register value_derived failed");
  ck_assert_msg(registry.register_definition(receiver), "register receiver failed");
  ck_assert_msg(registry.register_definition(overloads), "register overloads failed");

  DispatchEngine engine(registry);

  auto overrideR = engine.resolve(
      TypeID{0xC2ULL},
      "ping",
      OperationScope::Object,
      { TypeID{0x1001ULL} },
      1,
      true);
  ck_assert_msg(overrideR, "dispatch override failed: %s", result_message(overrideR));
  ck_assert_int_eq(overrideR.value->owner_type.v, 0xC2ULL);

  auto inheritedR = engine.resolve(
      TypeID{0xC2ULL},
      "pong",
      OperationScope::Object,
      { TypeID{0x1002ULL} },
      1,
      true);
  ck_assert_msg(inheritedR, "dispatch inherited failed: %s", result_message(inheritedR));
  ck_assert_int_eq(inheritedR.value->owner_type.v, 0xC1ULL);

  auto subtypeR = engine.resolve(
      receiver.type_id,
      "accept",
      OperationScope::Object,
      { value_derived.type_id },
      1,
      false);
  ck_assert_msg(subtypeR, "dispatch subtype failed: %s", result_message(subtypeR));
  ck_assert_int_eq(subtypeR.value->owner_type.v, receiver.type_id.v);

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

START_TEST(test_inheritance_resolver_exceptions_stay_in_results)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto def = make_definition(TypeID{0xE101ULL}, "ResolverFailure", "Demo");
  ck_assert_msg(registry.register_definition(def), "register definition failed");

  OperationRegistry operations(registry, [](TypeID) -> std::vector<TypeID> {
    throw std::runtime_error("resolver failure");
  });
  auto operationsR = operations.list_operations(def.type_id, OperationScope::Object, true);
  ck_assert_msg(!operationsR, "expected resolver failure result");
  ck_assert_int_eq(static_cast<int>(operationsR.error->code),
                   static_cast<int>(ErrorCode::Internal));
  ck_assert_str_eq(operationsR.error->message.c_str(), "resolver failure");

  DispatchEngine dispatch(registry, [](TypeID) -> std::vector<TypeID> {
    throw 7;
  });
  auto dispatchR = dispatch.resolve(def.type_id, "missing", OperationScope::Object, {}, 0, true);
  ck_assert_msg(!dispatchR, "expected non-standard resolver failure result");
  ck_assert_int_eq(static_cast<int>(dispatchR.error->code),
                   static_cast<int>(ErrorCode::Internal));
  ck_assert_str_eq(dispatchR.error->message.c_str(), "inheritance resolver failed");
}
END_TEST

Suite* refract_registry_suite(void) {
  Suite* s = suite_create("RefractRegistry");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_refract_lookups_return_typed_not_found);
  tcase_add_test(tc, test_schema_registry_validation_and_decode_errors_are_typed);
  tcase_add_test(tc, test_schema_registry_roundtrip);
  tcase_add_test(tc, test_schema_registry_supersedes_chain);
  tcase_add_test(tc, test_schema_registry_structured_metadata_roundtrip);
  tcase_add_test(tc, test_schema_registry_collection_metadata_roundtrip);
  tcase_add_test(tc, test_schema_registry_inheritance_metadata_roundtrip);
  tcase_add_test(tc, test_schema_registry_constraint_metadata_roundtrip);
  tcase_add_test(tc, test_schema_registry_effects_and_documentation_roundtrip);
  tcase_add_test(tc, test_schema_registry_rejects_invalid_relationship_constraint_bounds);
  tcase_add_test(tc, test_schema_registry_legacy_relationship_inheritance_fallback);
  tcase_add_test(tc, test_generic_instance_type_id_deterministic);
  tcase_add_test(tc, test_generic_instance_registry_roundtrip);
  tcase_add_test(tc, test_generic_pack_contracts_validate_before_persistence);
  tcase_add_test(tc, test_generic_pack_contracts_reject_corrupt_stored_instance);
  tcase_add_test(tc, test_astra_shape_contracts_validate_and_roundtrip);
  tcase_add_test(tc, test_resolve_or_register_propagates_corrupt_registry_data);
  tcase_add_test(tc, test_scoped_type_registry_promotion);
  tcase_add_test(tc, test_operation_registry_scope_and_inheritance);
  tcase_add_test(tc, test_dispatch_resolution);
  tcase_add_test(tc, test_inheritance_resolver_exceptions_stay_in_results);

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
