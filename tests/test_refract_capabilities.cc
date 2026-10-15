extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "refract/operation_registry.h"
#include "refract/schema_registry.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <unistd.h>
#include <vector>

using namespace referee;
using namespace iris::refract;

namespace {

struct TempStore {
  TempStore() {
    char pattern[] = "/tmp/iris-refract-capabilities-XXXXXX";
    auto* created = ::mkdtemp(pattern);
    ck_assert_ptr_nonnull(created);
    directory = created;
  }
  ~TempStore() { std::filesystem::remove_all(directory); }
  std::string path() const { return directory + "/registry"; }
  std::string directory;
};

nlohmann::json definition_payload() {
  return {{"type_id", 0xE103ULL}, {"name", "CapabilityExample"},
          {"namespace", "Test"}, {"version", 1},
          {"operations", nlohmann::json::array({{{"name", "inspect"}, {"scope", "object"}}})}};
}

void assert_requirements(const OperationDefinition& operation) {
  ck_assert_uint_eq(operation.required_capabilities.size(), 2);
  ck_assert_str_eq(operation.required_capabilities[0].c_str(), "example.inspect");
  ck_assert_str_eq(operation.required_capabilities[1].c_str(), "example.read");
}

} // namespace

START_TEST(test_malformed_stored_capabilities_are_corrupt)
{
  const std::vector<nlohmann::json> invalid = {
      nullptr, nlohmann::json::object(), {{"grant", "example.inspect"}},
      "example.inspect", 42, true,
      nlohmann::json::array({"example.inspect", nullptr}),
      nlohmann::json::array({"example.inspect", 1}),
      nlohmann::json::array({nlohmann::json::array()}),
      nlohmann::json::array({nlohmann::json::object()})};
  for (std::size_t i = 0; i < invalid.size(); ++i) {
    SqliteStore store(SqliteConfig{.filename=":memory:", .enable_wal=false});
    ck_assert(store.open());
    ck_assert(store.ensure_schema());
    auto payload = definition_payload();
    payload["operations"][0]["capabilities"] = invalid[i];
    auto stored = store.create_object(kTypeDefinitionType, ObjectID{}, nlohmann::json::to_cbor(payload));
    ck_assert(stored);
    SchemaRegistry schemas(store);
    auto decoded = schemas.get_definition_by_id(stored.value->ref.id);
    ck_assert_msg(!decoded, "malformed capabilities case %zu was accepted", i);
    ck_assert_int_eq(static_cast<int>(decoded.error->code), static_cast<int>(ErrorCode::CorruptData));
    OperationRegistry operations(schemas);
    auto listed = operations.list_operations(TypeID{0xE103ULL}, OperationScope::Object, false);
    ck_assert_msg(!listed, "malformed capabilities case %zu was listed", i);
    ck_assert_int_eq(static_cast<int>(listed.error->code), static_cast<int>(ErrorCode::CorruptData));
  }
}
END_TEST

START_TEST(test_missing_empty_and_string_capabilities_remain_valid)
{
  for (int kind = 0; kind < 3; ++kind) {
    SqliteStore store(SqliteConfig{.filename=":memory:", .enable_wal=false});
    ck_assert(store.open());
    ck_assert(store.ensure_schema());
    auto payload = definition_payload();
    if (kind == 1) payload["operations"][0]["capabilities"] = nlohmann::json::array();
    if (kind == 2) {
      payload["operations"][0]["capabilities"] = {"example.inspect", "example.read"};
    }
    auto stored = store.create_object(kTypeDefinitionType, ObjectID{}, nlohmann::json::to_cbor(payload));
    ck_assert(stored);
    SchemaRegistry schemas(store);
    auto decoded = schemas.get_definition_by_id(stored.value->ref.id);
    ck_assert_msg(decoded, "valid capabilities case %d was rejected", kind);
    ck_assert_uint_eq(decoded.value->definition.operations.size(), 1);
    const auto& operation = decoded.value->definition.operations[0];
    if (kind == 2) assert_requirements(operation);
    else ck_assert(operation.required_capabilities.empty());
    OperationRegistry operations(schemas);
    auto listed = operations.list_operations(TypeID{0xE103ULL}, OperationScope::Object, false);
    ck_assert_msg(listed, "valid capabilities case %d could not be listed", kind);
    ck_assert_uint_eq(listed.value->size(), 1);
    if (kind == 2) assert_requirements(listed.value->at(0));
    else ck_assert(listed.value->at(0).required_capabilities.empty());
  }
}
END_TEST

START_TEST(test_capabilities_survive_reopen_and_inherited_listing)
{
  TempStore temporary;
  const TypeID base_type{0xE10301ULL};
  const TypeID child_type{0xE10302ULL};
  ObjectID base_definition{};
  {
    SqliteStore store(SqliteConfig{.filename=temporary.path(), .enable_wal=false});
    ck_assert(store.open());
    ck_assert(store.ensure_schema());
    SchemaRegistry schemas(store);
    TypeDefinition base;
    base.type_id = base_type;
    base.name = "CapabilityBase";
    base.namespace_name = "Test";
    OperationDefinition operation;
    operation.name = "inspect";
    operation.scope = OperationScope::Object;
    operation.required_capabilities = {"example.inspect", "example.read"};
    base.operations.push_back(operation);
    auto registered = schemas.register_definition(base);
    ck_assert(registered);
    base_definition = registered.value->ref.id;
    TypeDefinition child;
    child.type_id = child_type;
    child.name = "CapabilityChild";
    child.namespace_name = "Test";
    child.base_types = {base_type};
    ck_assert(schemas.register_definition(child));
    ck_assert(store.close());
  }
  {
    SqliteStore store(SqliteConfig{.filename=temporary.path(), .enable_wal=false});
    ck_assert(store.open());
    ck_assert(store.ensure_schema());
    SchemaRegistry schemas(store);
    auto by_id = schemas.get_definition_by_id(base_definition);
    auto by_type = schemas.get_latest_definition_by_type(base_type);
    ck_assert(by_id);
    ck_assert(by_type);
    assert_requirements(by_id.value->definition.operations.at(0));
    assert_requirements(by_type.value->definition.operations.at(0));
    OperationRegistry operations(schemas);
    auto direct = operations.list_operations(base_type, OperationScope::Object, false);
    auto inherited = operations.list_operations(child_type, OperationScope::Object, true);
    auto own = operations.list_operations(child_type, OperationScope::Object, false);
    ck_assert(direct && inherited && own);
    ck_assert_uint_eq(direct.value->size(), 1);
    ck_assert_uint_eq(inherited.value->size(), 1);
    ck_assert(own.value->empty());
    assert_requirements(direct.value->at(0));
    assert_requirements(inherited.value->at(0));
    ck_assert_str_eq(inherited.value->at(0).name.c_str(), "inspect");
    ck_assert(store.close());
  }
}
END_TEST

int main() {
  Suite* suite = suite_create("RefractCapabilities");
  TCase* tests = tcase_create("persisted_metadata");
  tcase_add_test(tests, test_malformed_stored_capabilities_are_corrupt);
  tcase_add_test(tests, test_missing_empty_and_string_capabilities_remain_valid);
  tcase_add_test(tests, test_capabilities_survive_reopen_and_inherited_listing);
  suite_add_tcase(suite, tests);
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
