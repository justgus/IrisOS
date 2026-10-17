#include "contract_test_support.h"
#include "refract/standard_contracts.h"
#include "refract/contract_inspection.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <fstream>
#include <limits>
#include <sys/wait.h>

using namespace contract_test;
namespace {
ContractValue value(TypeID type, const nlohmann::json& x) {
  return {type, nlohmann::json::to_cbor({{"value", x}})};
}
nlohmann::json result_value(const Result<ContractValues>& result) {
  ok(result); ck_assert_uint_eq(result.value->size(), 1);
  return nlohmann::json::from_cbor(result.value->front().payload).at("value");
}
std::string shell(const Disk& disk, const std::string& script) {
  const auto script_path = disk.directory + "/commands";
  { std::ofstream out(script_path); out << script; }
  const auto command = "../bin/conch --db '" + disk.path() + "' < '" + script_path + "' 2>&1";
  FILE* pipe = ::popen(command.c_str(), "r"); ck_assert_ptr_nonnull(pipe);
  std::string output; char buffer[1024];
  while (std::fgets(buffer, sizeof(buffer), pipe)) output += buffer;
  const auto status = ::pclose(pipe);
  ck_assert_msg(status == 0, "Conch failed (%d): %s", status, output.c_str()); return output;
}
}

START_TEST(catalog_bootstrap_and_inspection_are_idempotent) {
  SqliteStore store({":memory:"}); ok(store.open()); SchemaRegistry schema(store);
  ok(bootstrap_core_schema(schema)); OperationCatalog catalog(store);
  auto first = bootstrap_standard_contracts(schema, catalog); ok(first);
  ck_assert_uint_eq(first.value->inserted, 8);
  auto second = bootstrap_standard_contracts(schema, catalog); ok(second);
  ck_assert_uint_eq(second.value->inserted, 0); ck_assert_uint_eq(second.value->existing, 8);
  auto all = catalog.list(); ok(all); ck_assert_uint_eq(all.value->size(), 3);
  auto filtered = inspect_contract_catalog(catalog, "arithmetic"); ok(filtered);
  const auto contracts = nlohmann::json::parse(*filtered.value);
  ck_assert_uint_eq(contracts.size(), 1); ck_assert(contracts[0]["name"] == "add");
  ck_assert_uint_eq(contracts[0]["inputs"].size(), 2);
  auto view = inspect_type_contracts(schema, catalog, {0x1002}); ok(view);
  auto again = inspect_type_contracts(schema, catalog, {0x1002}); ok(again);
  ck_assert(*again.value == *view.value);
  auto claims = nlohmann::json::parse(*view.value); ck_assert_uint_eq(claims.size(), 3);
  for (const auto& claim : claims) {
    ck_assert(claim["binding"]["route"] == "local");
    ck_assert_uint_eq(claim["owner"].get<std::uint64_t>(), 0x1002);
    ck_assert(claim.contains("effects") && claim.contains("capabilities"));
    ck_assert(claim["contract"].contains("summary"));
  }
} END_TEST

START_TEST(primitive_callbacks_and_numeric_errors) {
  SqliteStore store({":memory:"}); ok(store.open()); SchemaRegistry schema(store);
  ok(bootstrap_core_schema(schema)); OperationCatalog catalog(store);
  ok(bootstrap_standard_contracts(schema, catalog)); ContractDispatcher dispatcher(schema, catalog);
  ok(bind_standard_contracts(dispatcher));
  const auto add = standard_contract_key(StandardContract::Add);
  auto sum = dispatcher.invoke({0x1002}, add, {value({0x1002}, 4), value({0x1002}, 5)}, {});
  ck_assert_uint_eq(result_value(sum).get<std::uint64_t>(), 9);
  auto real = dispatcher.invoke({0x1008}, add, {value({0x1008}, 1.25), value({0x1008}, 2.5)}, {});
  ck_assert_double_eq(result_value(real).get<double>(), 3.75);
  error(dispatcher.invoke({0x1002}, add, {value({0x1002}, UINT64_MAX), value({0x1002}, 1)}, {}), ErrorCode::InvalidArgument);
  error(dispatcher.invoke({0x1008}, add, {value({0x1008}, std::numeric_limits<double>::max()),
                                       value({0x1008}, std::numeric_limits<double>::max())}, {}), ErrorCode::InvalidArgument);
  error(dispatcher.invoke({0x1008}, add, {value({0x1008}, std::numeric_limits<double>::infinity()),
                                       value({0x1008}, 1.0)}, {}), ErrorCode::InvalidArgument);
  error(dispatcher.invoke({0x1002}, add, {value({0x1002}, "bad"), value({0x1002}, 1)}, {}), ErrorCode::InvalidArgument);
  error(dispatcher.invoke({0x1002}, add, {value({0x1002}, 1), value({0x1008}, 2.0)}, {}), ErrorCode::NotFound);
  auto bytes = dispatcher.invoke({0x1007}, standard_contract_key(StandardContract::ToString), {}, {},
                                  value({0x1007}, nlohmann::json::array({0, 15, 255})));
  ck_assert(result_value(bytes) == "000fff");
  error(dispatcher.invoke({0x1007}, standard_contract_key(StandardContract::ToString), {}, {},
                           value({0x1007}, nlohmann::json::array({256}))), ErrorCode::InvalidArgument);
  auto ordered = dispatcher.invoke({0x1002}, standard_contract_key(StandardContract::Compare),
                                    {value({0x1002}, UINT64_MAX - 1)}, {}, value({0x1002}, UINT64_MAX));
  ck_assert_double_eq(result_value(ordered).get<double>(), 1.0);
  const std::vector<nlohmann::json> examples = {"hello", 7, true, "123456789abcdef0123456789abcdef0", 0x1002, 2, "bytes", 2.5};
  const std::vector<std::string> expected = {"hello", "7", "true", "123456789abcdef0123456789abcdef0", "0x1002", "2", "bytes", "2.5"};
  for (std::size_t i = 0; i < examples.size(); ++i) {
    TypeID type{0x1001 + i}; auto receiver = value(type, examples[i]);
    auto text = dispatcher.invoke(type, standard_contract_key(StandardContract::ToString), {}, {}, receiver);
    const auto actual = result_value(text).get<std::string>(); ck_assert_str_eq(actual.c_str(), expected[i].c_str());
    auto equal = dispatcher.invoke(type, standard_contract_key(StandardContract::Compare), {receiver}, {}, receiver);
    ck_assert_double_eq(result_value(equal).get<double>(), 0.0);
  }
} END_TEST

START_TEST(reopen_binding_and_result_separation) {
  Disk disk; ObjectRef source, destination;
  {
    SqliteStore store({disk.path()}); ok(store.open()); SchemaRegistry schema(store);
    ok(bootstrap_core_schema(schema)); OperationCatalog catalog(store); ok(bootstrap_standard_contracts(schema, catalog));
    auto a = store.create_object({0x1002}, {}, value({0x1002}, 4).payload); ok(a); source = a.value->ref;
    auto b = store.create_object({0x1002}, {}, value({0x1002}, 99).payload); ok(b); destination = b.value->ref;
    ok(store.close());
  }
  SqliteStore store({disk.path()}); ok(store.open()); SchemaRegistry schema(store); OperationCatalog catalog(store);
  auto boot = bootstrap_standard_contracts(schema, catalog); ok(boot); ck_assert_uint_eq(boot.value->inserted, 0);
  ContractDispatcher dispatcher(schema, catalog);
  const auto key = standard_contract_key(StandardContract::ToString);
  error(dispatcher.invoke_object(source.id, store, key, {}, {}), ErrorCode::FailedPrecondition);
  ok(bind_standard_contracts(dispatcher));
  auto text = dispatcher.invoke_object(source.id, store, key, {}, {});
  ck_assert(result_value(text) == "4");
  auto result = dispatcher.invoke({0x1002}, standard_contract_key(StandardContract::Add),
                                  {value({0x1002}, 4), value({0x1002}, 5)}, {});
  ck_assert_uint_eq(result_value(result).get<std::uint64_t>(), 9);
  auto a = store.get_latest(source.id); ok(a); ck_assert(a.value->ref == source);
  auto b = store.get_latest(destination.id); ok(b); ck_assert(b.value->ref == destination);
  ck_assert(b.value->payload_cbor == value({0x1002}, 99).payload);
} END_TEST

START_TEST(conch_inspection_and_legacy_calls) {
  Disk disk; ObjectID id;
  {
    SqliteStore store({disk.path()}); ok(store.open()); SchemaRegistry schema(store); ok(bootstrap_core_schema(schema));
    auto def = schema.get_definition_by_type({0x1002}); ok(def);
    auto object = store.create_object({0x1002}, def.value->ref.id, value({0x1002}, 42).payload);
    ok(object); id = object.value->ref.id; ok(store.close());
  }
  auto output = shell(disk, "ops --catalog --category arithmetic\nops Refract::U64 --contracts\n"
                           "ops Refract::U64 --declared\ncall " + id.to_hex() + " to_string\ncall " + id.to_hex() + " compare 42\nquit\n");
  ck_assert_msg(output.find("error:") == std::string::npos, "%s", output.c_str());
  for (const auto& expected : {"\"category\": \"arithmetic\"", "\"route\": \"local\"", "result 42", "result 0", "call ok"})
    ck_assert_msg(output.find(expected) != std::string::npos, "missing %s: %s", expected, output.c_str());
  auto reopened = shell(disk, "ops --catalog --capability absent\nops Refract::U64 --contracts\nquit\n");
  ck_assert_msg(reopened.find("error:") == std::string::npos, "%s", reopened.c_str());
  ck_assert(reopened.find("\"operation\": \"add\"") != std::string::npos);
  auto malformed = shell(disk, "ops --catalog --category\nquit\n");
  ck_assert(malformed.find("error: ops --catalog") != std::string::npos);
} END_TEST

int main() {
  auto* tc = tcase_create("standard"); tcase_set_timeout(tc, 30);
  tcase_add_test(tc, catalog_bootstrap_and_inspection_are_idempotent);
  tcase_add_test(tc, primitive_callbacks_and_numeric_errors);
  tcase_add_test(tc, reopen_binding_and_result_separation);
  tcase_add_test(tc, conch_inspection_and_legacy_calls);
  return run(tc, "StandardContracts");
}
