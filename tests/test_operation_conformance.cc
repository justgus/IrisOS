#include "contract_test_support.h"
#include "refract/operation_registry.h"
#include <nlohmann/json.hpp>

using namespace contract_test;
namespace {
TypeDefinition definition() {
  TypeDefinition d; d.type_id = {0x2701}; d.name = "Value";
  OperationDefinition op; op.name = "sum"; op.scope = OperationScope::Class;
  op.signature.params = {{"a", d.type_id, false}, {"b", d.type_id, false}};
  op.signature.outputs = {{"result", d.type_id, false}};
  op.conformances = {{sample().key, {{"Self", d.type_id}}}};
  op.binding = OperationBinding{ObjectID::from_hex("27000000000000000000000000000011"), 1,
                                 ExecutionRoute::Local, ""};
  d.operations.push_back(op); return d;
}
}
START_TEST(reject_incompatible_before_publishing) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store);
  ok(catalog.register_contract(sample())); SchemaRegistry schema(store);
  for (int i = 0; i < 10; ++i) {
    auto d = definition(); auto& op = d.operations.front();
    switch (i) {
      case 0: op.scope = OperationScope::Object; break;
      case 1: op.signature.outputs[0].type = {42}; break;
      case 2: op.signature.params[1].optional = true; break;
      case 3: op.conformances[0].substitutions.clear(); break;
      case 4: op.conformances[0].substitutions["Self"] = {42}; break;
      case 5: op.conformances[0].contract.version = 99; break;
      case 6: op.binding.reset(); break;
      case 7: op.binding->destination = "unexpected"; break;
      case 8: op.conformances.push_back(op.conformances.front()); break;
      case 9: op.conformances[0].substitutions["Extra"] = {1}; break;
    }
    error(schema.register_definition(d), ErrorCode::InvalidArgument);
  }
  auto records = store.list_by_type(kTypeDefinitionType); ok(records); ck_assert(records.value->empty());
  auto d = definition(); d.operations[0].binding->route = ExecutionRoute::Service;
  d.operations[0].binding->destination = "service:math"; ok(schema.register_definition(d));
} END_TEST

START_TEST(inherited_and_object_discovery) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store);
  ok(catalog.register_contract(sample())); SchemaRegistry schema(store);
  auto parent = schema.register_definition(definition()); ok(parent);
  TypeDefinition child; child.type_id = {0x2702}; child.name = "Child"; child.base_types = {{0x2701}};
  ok(schema.register_definition(child)); OperationRegistry ops(schema);
  auto inherited = ops.list_conformances(child.type_id, catalog, sample().key); ok(inherited);
  ck_assert_uint_eq(inherited.value->size(), 1);
  ck_assert_uint_eq(inherited.value->front().owner.v, 0x2701);
  ck_assert_uint_eq(inherited.value->front().depth, 1);
  auto declared = ops.list_conformances(child.type_id, catalog, {}, false); ok(declared);
  ck_assert(declared.value->empty());
  auto object = store.create_object(child.type_id, {}, {}); ok(object);
  auto view = ops.list_object_conformances(object.value->ref.id, store, catalog); ok(view);
  ck_assert_uint_eq(view.value->size(), 1);
  auto claimless = definition(); claimless.type_id = {0x2703}; claimless.name = "SameName";
  claimless.operations[0].conformances.clear(); claimless.operations[0].binding.reset();
  ok(schema.register_definition(claimless));
  auto none = ops.list_conformances(claimless.type_id, catalog); ok(none); ck_assert(none.value->empty());
} END_TEST

START_TEST(persistence_and_malformed_metadata) {
  Disk disk; nlohmann::json json;
  {
    SqliteStore store({disk.path()}); ok(store.open()); OperationCatalog catalog(store);
    ok(catalog.register_contract(sample())); SchemaRegistry schema(store);
    auto record = schema.register_definition(definition()); ok(record);
    auto raw = store.get_object(record.value->ref); ok(raw);
    json = nlohmann::json::from_cbor(raw.value->payload_cbor); ok(store.close());
  }
  {
    SqliteStore store({disk.path()}); ok(store.open()); OperationCatalog catalog(store);
    SchemaRegistry schema(store); OperationRegistry ops(schema);
    auto view = ops.list_conformances({0x2701}, catalog); ok(view);
    ck_assert_uint_eq(view.value->size(), 1);
    ck_assert_uint_eq(view.value->front().conformance.substitutions.at("Self").v, 0x2701);
    ck_assert_uint_eq(view.value->front().operation.binding->version, 1);
  }
  for (int i = 0; i < 9; ++i) {
    SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store);
    ok(catalog.register_contract(sample())); auto bad = json;
    auto& op = bad["operations"][0];
    switch (i) {
      case 0: op["conformances"] = nullptr; break;
      case 1: op["binding"]["version"] = -1; break;
      case 2: op["binding"]["route"] = "unknown"; break;
      case 3: op["conformances"][0]["substitutions"] = nlohmann::json::array(); break;
      case 4: op["conformances"][0]["id"] = "bad"; break;
      case 5: op["conformances"][0]["substitutions"]["Self"] = 0; break;
      case 6: op["scope"] = "unknown"; break;
      case 7: op["signature"]["outputs"] = nullptr; break;
      case 8: op["signature"]["params"][0]["type_id"] = -1; break;
    }
    ok(store.create_object(kTypeDefinitionType, {}, nlohmann::json::to_cbor(bad)));
    SchemaRegistry schema(store); error(schema.get_latest_definition_by_type({0x2701}), ErrorCode::CorruptData);
  }
} END_TEST

int main() {
  auto* tc = tcase_create("conformance");
  tcase_add_test(tc, reject_incompatible_before_publishing);
  tcase_add_test(tc, inherited_and_object_discovery);
  tcase_add_test(tc, persistence_and_malformed_metadata);
  return run(tc, "OperationConformance");
}
