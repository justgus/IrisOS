#include "contract_test_support.h"
#include "refract/contract_dispatch.h"
#include <nlohmann/json.hpp>

using namespace contract_test;
namespace {
constexpr TypeID owner{0x2710}, number{0x1002}, text{0x1001};
ObjectID implementation() { return ObjectID::from_hex("27000000000000000000000000000022"); }
OperationContract contract() {
  auto c = sample(); c.type_parameters = {"T"};
  for (auto* ps : {&c.inputs, &c.outputs}) for (auto& p : *ps) p.type_parameter = "T";
  return c;
}
OperationDefinition operation(TypeID type = number) {
  OperationDefinition op; op.name = "arbitrary_name"; op.scope = OperationScope::Class;
  op.signature.params = {{"a", type, false}, {"b", type, false}};
  op.signature.outputs = {{"result", type, false}};
  op.conformances = {{contract().key, {{"T", type}}}};
  op.binding = OperationBinding{implementation(), 1, ExecutionRoute::Local, ""}; return op;
}
TypeDefinition definition(std::vector<OperationDefinition> ops) {
  TypeDefinition d; d.type_id = owner; d.name = "Operations"; d.operations = std::move(ops); return d;
}
ContractValue value(TypeID type = number) { return {type, nlohmann::json::to_cbor({{"value", 3}})}; }
void setup(OperationCatalog& catalog, SchemaRegistry& schema, std::vector<OperationDefinition> ops) {
  ok(catalog.register_contract(contract())); ok(schema.register_definition(definition(std::move(ops))));
}
}

START_TEST(all_inputs_contract_identity_and_ambiguity) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store); SchemaRegistry schema(store);
  setup(catalog, schema, {operation(number), operation(text)}); ContractDispatcher dispatcher(schema, catalog);
  auto match = dispatcher.resolve(owner, contract().key, {number, number}); ok(match);
  ck_assert_uint_eq(match.value->operation.signature.outputs[0].type.v, number.v);
  error(dispatcher.resolve(owner, contract().key, {number, text}), ErrorCode::NotFound);
  error(dispatcher.resolve(owner, contract().key, {number}), ErrorCode::NotFound);
  error(dispatcher.resolve(owner, contract().key, {number, {0}}), ErrorCode::InvalidArgument);
  auto other = contract(); other.key.id = ObjectID::random(); ok(catalog.register_contract(other));
  error(dispatcher.resolve(owner, other.key, {number, number}), ErrorCode::NotFound);
  auto duplicate = operation(); duplicate.name = "same_contract_different_name";
  auto d = definition({operation(), duplicate}); d.version = 2; ok(schema.register_definition(d));
  error(dispatcher.resolve(owner, contract().key, {number, number}), ErrorCode::FailedPrecondition);
} END_TEST

START_TEST(subtypes_inheritance_optional_and_generic_identity) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store); SchemaRegistry schema(store);
  auto c = contract(); c.inputs[1].optional = true; ok(catalog.register_contract(c));
  auto op = operation(); op.signature.params[1].optional = true;
  ok(schema.register_definition(definition({op})));
  TypeDefinition base; base.name = "Number"; base.type_id = number; ok(schema.register_definition(base));
  TypeDefinition child; child.name = "Derived"; child.type_id = {0x2711}; child.base_types = {number};
  ok(schema.register_definition(child));
  TypeDefinition inherited; inherited.name = "Inherited"; inherited.type_id = {0x2712}; inherited.base_types = {owner};
  ok(schema.register_definition(inherited)); ContractDispatcher dispatcher(schema, catalog);
  ok(dispatcher.resolve(inherited.type_id, c.key, {child.type_id, child.type_id}));
  ok(dispatcher.resolve(owner, c.key, {number}));
  GenericInstance a; a.base_type = {0x4352415400000001ULL}; a.args = {{GenericArgKind::Type, number, {}, "", {}}};
  GenericInstance b = a; b.args[0].type_id = text;
  TypeDefinition generic_schema; generic_schema.type_id = kTypeGenericInstanceType;
  generic_schema.name = "GenericInstance"; ok(schema.register_definition(generic_schema));
  GenericRegistry generics(schema, store);
  auto ar = generics.register_instance(a); ok(ar); auto br = generics.register_instance(b); ok(br);
  auto generic = operation(ar.value->instance.instance_type); generic.signature.params[1].optional = true;
  auto d = definition({generic}); d.version = 2; ok(schema.register_definition(d));
  const auto at = ar.value->instance.instance_type, bt = br.value->instance.instance_type;
  ok(dispatcher.resolve(owner, c.key, {at, at}));
  error(dispatcher.resolve(owner, c.key, {at, bt}), ErrorCode::NotFound);
} END_TEST

START_TEST(authority_routes_and_output_validation) {
  SqliteStore store({":memory:"}); ok(store.open()); OperationCatalog catalog(store); SchemaRegistry schema(store);
  auto op = operation(); op.required_capabilities = {"math.use"}; setup(catalog, schema, {op});
  ContractDispatcher dispatcher(schema, catalog); const auto inputs = ContractValues{value(), value()};
  int calls = 0;
  ok(dispatcher.bind(implementation(), 1, [&](const auto&, const auto&) {
    ++calls; return Result<ContractValues>::ok({value()});
  }));
  error(dispatcher.invoke(owner, contract().key, inputs, {}), ErrorCode::FailedPrecondition);
  ck_assert_int_eq(calls, 0);
  auto result = dispatcher.invoke(owner, contract().key, inputs, {"math.use"}); ok(result);
  ck_assert_int_eq(calls, 1); ck_assert_uint_eq(result.value->size(), 1);
  ck_assert(inputs[0].payload == value().payload);
  error(dispatcher.bind(implementation(), 1, {}), ErrorCode::InvalidArgument);
  error(dispatcher.bind(implementation(), 1, [](const auto&, const auto&) {
    return Result<ContractValues>::ok({});
  }), ErrorCode::AlreadyExists);
  for (int i = 0; i < 4; ++i) {
    ContractDispatcher bad(schema, catalog);
    ok(bad.bind(implementation(), 1, [i](const auto&, const auto&) -> Result<ContractValues> {
      if (i == 0) return Result<ContractValues>::ok({});
      if (i == 1) return Result<ContractValues>::ok({value(text)});
      if (i == 2) return Result<ContractValues>::err(ErrorCode::IoError, "implementation failure");
      throw std::runtime_error("callback failure");
    }));
    const auto expected = i < 2 ? ErrorCode::FailedPrecondition : i == 2 ? ErrorCode::IoError : ErrorCode::Internal;
    error(bad.invoke(owner, contract().key, inputs, {"math.use"}), expected);
  }
  op.binding->route = ExecutionRoute::Service; op.binding->destination = "service:math";
  auto d = definition({op}); d.version = 2; ok(schema.register_definition(d));
  error(dispatcher.invoke(owner, contract().key, inputs, {"math.use"}), ErrorCode::Unsupported);
} END_TEST

START_TEST(reopen_requires_runtime_rebinding_and_preserves_objects) {
  Disk disk; ObjectRef source, destination;
  {
    SqliteStore store({disk.path()}); ok(store.open()); OperationCatalog catalog(store); SchemaRegistry schema(store);
    setup(catalog, schema, {operation()});
    auto a = store.create_object(number, {}, value().payload); ok(a); source = a.value->ref;
    auto b = store.create_object(number, {}, value().payload); ok(b); destination = b.value->ref;
    ok(store.close());
  }
  SqliteStore store({disk.path()}); ok(store.open()); OperationCatalog catalog(store); SchemaRegistry schema(store);
  ContractDispatcher dispatcher(schema, catalog); ContractValues inputs{value(), value()};
  error(dispatcher.invoke(owner, contract().key, inputs, {}), ErrorCode::FailedPrecondition);
  ok(dispatcher.bind(implementation(), 1, [](const auto&, const auto&) {
    return Result<ContractValues>::ok({value()});
  }));
  ok(dispatcher.invoke(owner, contract().key, inputs, {}));
  for (auto ref : {source, destination}) {
    auto record = store.get_latest(ref.id); ok(record);
    ck_assert(record.value->ref == ref); ck_assert(record.value->payload_cbor == value().payload);
  }
  error(dispatcher.invoke_object(source.id, store, contract().key, inputs, {}), ErrorCode::NotFound);
} END_TEST

int main() {
  auto* tc = tcase_create("dispatch");
  tcase_add_test(tc, all_inputs_contract_identity_and_ambiguity);
  tcase_add_test(tc, subtypes_inheritance_optional_and_generic_identity);
  tcase_add_test(tc, authority_routes_and_output_validation);
  tcase_add_test(tc, reopen_requires_runtime_rebinding_and_preserves_objects);
  return run(tc, "ContractDispatch");
}
