#include "contract_test_support.h"
#include <nlohmann/json.hpp>

using namespace contract_test;

START_TEST(identity_versions_and_filters) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store);
  auto c = sample(); c.required_capabilities = {"math.use"};
  c.effects = {{OperationEffectKind::Reads, "inputs", "read input values"}};
  c.documentation.examples = {"add(1,2) = 3"};
  ok(catalog.register_contract(c)); ok(catalog.register_contract(c));
  auto changed = c; changed.name = "renamed";
  error(catalog.register_contract(changed), ErrorCode::AlreadyExists);
  changed.key.version = 2; ok(catalog.register_contract(changed));
  changed.key.id = ObjectID::from_hex("27000000000000000000000000000002");
  changed.name = c.name; ok(catalog.register_contract(changed));
  auto all = catalog.list(); ok(all); ck_assert_uint_eq(all.value->size(), 3);
  auto filtered = catalog.list("arithmetic", "math.use"); ok(filtered);
  ck_assert_uint_eq(filtered.value->size(), 3);
  auto empty = catalog.list("rendering"); ok(empty); ck_assert(empty.value->empty());
  auto original = catalog.get(c.key); ok(original);
  ck_assert_str_eq(original.value->name.c_str(), "add");
  ck_assert_uint_eq(original.value->effects.size(), 1);
  ck_assert_uint_eq(original.value->documentation.examples.size(), 1);
  error(catalog.get({c.key.id, 3}), ErrorCode::NotFound);
} END_TEST

START_TEST(validation_is_atomic) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store);
  for (int i = 0; i < 9; ++i) {
    auto c = sample();
    switch (i) {
      case 0: c.key.id = {}; break;
      case 1: c.key.version = 0; break;
      case 2: c.inputs[0].type = {1}; break;
      case 3: c.inputs[0].type_parameter = "Unknown"; break;
      case 4: c.inputs[1].name = "left"; break;
      case 5: c.type_parameters.push_back("Self"); break;
      case 6: c.inputs[0].optional = true; break;
      case 7: c.documentation.summary.reset(); break;
      case 8: c.scope = static_cast<OperationScope>(42); break;
    }
    error(catalog.register_contract(c), ErrorCode::InvalidArgument);
  }
  auto all = catalog.list(); ok(all); ck_assert(all.value->empty());
  ok(store.close()); error(catalog.list(), ErrorCode::FailedPrecondition);
} END_TEST

START_TEST(reopen_and_corruption) {
  Disk disk; auto c = sample();
  {
    SqliteStore store({disk.path()}); ok(store.open()); OperationCatalog catalog(store);
    ok(catalog.register_contract(c)); ok(store.close());
  }
  SqliteStore store({disk.path()}); ok(store.open()); OperationCatalog catalog(store);
  auto loaded = catalog.get(c.key); ok(loaded);
  ck_assert_str_eq(loaded.value->inputs[1].type_parameter.c_str(), "Self");
  ok(catalog.register_contract(c));
  auto raw = store.list_by_type(kOperationContractType); ok(raw); ck_assert_uint_eq(raw.value->size(), 1);
  auto json = nlohmann::json::from_cbor(raw.value->front().payload_cbor);
  ok(store.close());
  for (const auto& field : {"version", "inputs", "effects", "capabilities", "type_parameters", "scope"}) {
    SqliteStore malformed({":memory:"}); ok(malformed.open());
    auto bad = json; bad[field] = nullptr;
    ok(malformed.create_object(kOperationContractType, {}, nlohmann::json::to_cbor(bad)));
    OperationCatalog corrupt(malformed); error(corrupt.list(), ErrorCode::CorruptData);
    error(corrupt.get(c.key), ErrorCode::CorruptData);
  }
} END_TEST

int main() {
  auto* tc = tcase_create("catalog");
  tcase_add_test(tc, identity_versions_and_filters);
  tcase_add_test(tc, validation_is_atomic);
  tcase_add_test(tc, reopen_and_corruption);
  return run(tc, "OperationCatalog");
}
