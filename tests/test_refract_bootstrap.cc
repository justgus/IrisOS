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
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>

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

const OperationDefinition* find_operation(const DefinitionRecord& record, const std::string& name) {
  for (const auto& op : record.definition.operations) {
    if (op.name == name) return &op;
  }
  return nullptr;
}

bool type_params_match(const DefinitionRecord& record, const std::vector<std::string>& params) {
  return record.definition.type_params == params;
}

bool payload_has_symbol(const referee::ObjectRecord& rec, const std::string& symbol) {
  try {
    auto j = nlohmann::json::from_cbor(rec.payload_cbor);
    return j.value("symbol", "") == symbol;
  } catch (const std::exception&) {
    return false;
  }
}

std::string make_temp_db_path() {
  char path[] = "/tmp/iris_caliper_catalog_XXXXXX";
  const int fd = mkstemp(path);
  if (fd >= 0) close(fd);
  std::string result(path);
  std::remove(result.c_str());
  return result;
}

void cleanup_temp_db(const std::string& path) {
  std::remove(path.c_str());
  std::remove((path + "-shm").c_str());
  std::remove((path + "-wal").c_str());
  const std::string segments = path + ".segments";
  std::remove((segments + "/segments/objects.seg").c_str());
  std::remove((segments + "/segments/edges.seg").c_str());
  std::remove((segments + "/segments/graph_changes.seg").c_str());
  std::remove((segments + "/indexes/objects_by_id.idx").c_str());
  std::remove((segments + "/indexes/objects_by_type.idx").c_str());
  std::remove((segments + "/indexes/edges_from.idx").c_str());
  std::remove((segments + "/indexes/edges_to.idx").c_str());
  rmdir((segments + "/segments").c_str());
  rmdir((segments + "/indexes").c_str());
  rmdir(segments.c_str());
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
  auto unit_definition = registry.get_latest_definition_by_type(TypeID{0x43414C5000000002ULL});
  ck_assert_msg(unit_definition, "latest Caliper::Unit definition lookup failed");
  ck_assert_uint_eq(unit_definition.value->definition.version, 2U);
  ck_assert(std::any_of(unit_definition.value->definition.fields.begin(),
                        unit_definition.value->definition.fields.end(),
                        [](const FieldDefinition& field) {
                          return field.name == "override_base" && !field.required
                              && field.type == TypeID{0x1003ULL};
                        }));

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

  struct ExpectedCollection {
    const char* name;
    std::vector<std::string> type_params;
    bool has_index;
  };
  const std::vector<ExpectedCollection> expected = {
    {"Array", {"T"}, true},
    {"List", {"T"}, true},
    {"Set", {"T"}, true},
    {"Map", {"K", "V"}, true},
    {"Tuple", {"Ts"}, true},
  };
  const std::vector<std::string> common_operations = {"size", "iterate", "contains"};

  for (const auto& collection : expected) {
    const std::string collection_name = std::string("Crate::") + collection.name;
    auto summary = find_type(types, "Crate", collection.name);
    ck_assert_msg(summary.has_value(), "%s missing", collection_name.c_str());

    auto defR = registry.get_definition_by_type(summary->type_id);
    ck_assert_msg(defR, "%s definition lookup failed: %s",
                  collection_name.c_str(), result_message(defR));

    const auto& record = defR.value.value();
    ck_assert_msg(type_params_match(record, collection.type_params),
                  "%s has unexpected type parameter labels", collection_name.c_str());
    for (const auto& operation : common_operations) {
      ck_assert_msg(type_has_operation(record, operation), "%s missing %s operation",
                    collection_name.c_str(), operation.c_str());
    }
    if (collection.has_index) {
      ck_assert_msg(type_has_operation(record, "index"), "%s missing index operation",
                    collection_name.c_str());
    }
  }

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_bootstrap_crate_set_index_signature_reopen)
{
  const std::string path = make_temp_db_path();
  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    SchemaRegistry registry(store);
    auto boot = bootstrap_core_schema(registry);
    ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));
    ck_assert_msg(store.close(), "close before reopen failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "reopen failed");
    SchemaRegistry registry(store);
    auto listR = registry.list_types();
    ck_assert_msg(listR, "list_types after reopen failed: %s", result_message(listR));
    auto set = find_type(listR.value.value(), "Crate", "Set");
    ck_assert_msg(set.has_value(), "Crate::Set missing after reopen");
    auto defR = registry.get_definition_by_type(set->type_id);
    ck_assert_msg(defR, "Set definition after reopen failed: %s", result_message(defR));
    const auto* index = find_operation(defR.value.value(), "index");
    ck_assert_msg(index, "Set missing index operation after reopen");
    ck_assert_msg(index->scope == OperationScope::Object, "Set index has unexpected scope");
    ck_assert_msg(index->signature.params.size() == 1, "Set index must have one parameter");
    ck_assert_msg(index->signature.outputs.size() == 1, "Set index must have one output");
    ck_assert_str_eq(index->signature.params[0].name.c_str(), "index");
    ck_assert_str_eq(index->signature.outputs[0].name.c_str(), "value");

    auto u64 = find_type(listR.value.value(), "Refract", "U64");
    auto bytes = find_type(listR.value.value(), "Refract", "Bytes");
    ck_assert_msg(u64.has_value(), "Refract::U64 missing after reopen");
    ck_assert_msg(bytes.has_value(), "Refract::Bytes missing after reopen");
    ck_assert(index->signature.params[0].type == u64->type_id);
    ck_assert(index->signature.outputs[0].type == bytes->type_id);
    ck_assert_msg(store.close(), "close after reopen failed");
  }
  cleanup_temp_db(path);
}
END_TEST

START_TEST(test_bootstrap_core_ops_on_primitives)
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

  auto string_type = find_type(types, "Refract", "String");
  auto u64_type = find_type(types, "Refract", "U64");
  ck_assert_msg(string_type.has_value(), "Refract::String missing");
  ck_assert_msg(u64_type.has_value(), "Refract::U64 missing");

  auto string_def = registry.get_definition_by_type(string_type->type_id);
  ck_assert_msg(string_def, "String definition lookup failed: %s", result_message(string_def));
  ck_assert_msg(type_has_operation(string_def.value.value(), "to_string"), "String missing to_string");
  ck_assert_msg(type_has_operation(string_def.value.value(), "print"), "String missing print");
  ck_assert_msg(type_has_operation(string_def.value.value(), "render"), "String missing render");
  ck_assert_msg(type_has_operation(string_def.value.value(), "compare"), "String missing compare");

  auto u64_def = registry.get_definition_by_type(u64_type->type_id);
  ck_assert_msg(u64_def, "U64 definition lookup failed: %s", result_message(u64_def));
  ck_assert_msg(type_has_operation(u64_def.value.value(), "to_string"), "U64 missing to_string");
  ck_assert_msg(type_has_operation(u64_def.value.value(), "print"), "U64 missing print");
  ck_assert_msg(type_has_operation(u64_def.value.value(), "render"), "U64 missing render");
  ck_assert_msg(type_has_operation(u64_def.value.value(), "compare"), "U64 missing compare");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_bootstrap_conch_types)
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

  ck_assert_msg(find_type(types, "Conch", "Session").has_value(), "Conch::Session missing");
  ck_assert_msg(find_type(types, "Conch", "Concho").has_value(), "Conch::Concho missing");
  ck_assert_msg(find_type(types, "Conch", "Alias").has_value(), "Conch::Alias missing");
  ck_assert_msg(find_type(types, "Conch", "IoHandleAlias").has_value(),
                "Conch::IoHandleAlias missing");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_bootstrap_astra_math_types)
{
  const std::string path = make_temp_db_path();
  SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto listR = registry.list_types();
  ck_assert_msg(listR, "list_types failed: %s", result_message(listR));
  const auto& types = listR.value.value();

  auto vector_type = find_type(types, "Astra", "Vector");
  auto matrix_type = find_type(types, "Astra", "Matrix");
  auto tensor_type = find_type(types, "Astra", "Tensor");

  auto float_type = find_type(types, "Astra", "Float");
  auto double_type = find_type(types, "Astra", "Double");
  ck_assert_msg(float_type.has_value(), "Astra::Float missing");
  ck_assert_msg(double_type.has_value(), "Astra::Double missing");
  ck_assert_msg(vector_type.has_value(), "Astra::Vector missing");
  ck_assert_msg(matrix_type.has_value(), "Astra::Matrix missing");
  ck_assert_msg(tensor_type.has_value(), "Astra::Tensor missing");

  auto vector_def = registry.get_definition_by_type(vector_type->type_id);
  ck_assert_msg(vector_def, "Astra::Vector definition lookup failed: %s", result_message(vector_def));
  ck_assert_msg(type_params_match(vector_def.value.value(), {"T", "N"}),
                "Astra::Vector type params missing");

  auto matrix_def = registry.get_definition_by_type(matrix_type->type_id);
  ck_assert_msg(matrix_def, "Astra::Matrix definition lookup failed: %s", result_message(matrix_def));
  ck_assert_msg(type_params_match(matrix_def.value.value(), {"T", "R", "C"}),
                "Astra::Matrix type params missing");

  auto tensor_def = registry.get_definition_by_type(tensor_type->type_id);
  ck_assert_msg(tensor_def, "Astra::Tensor definition lookup failed: %s", result_message(tensor_def));
  ck_assert_msg(type_params_match(tensor_def.value.value(), {"T", "Dims..."}),
                "Astra::Tensor type params missing");

  ck_assert_msg(store.close(), "close failed");

  SqliteStore reopened(SqliteConfig{ .filename=path, .enable_wal=false });
  ck_assert_msg(reopened.open(), "reopen failed");
  ck_assert_msg(reopened.ensure_schema(), "reopened ensure_schema failed");
  SchemaRegistry reopened_registry(reopened);
  auto reopened_float = reopened_registry.get_definition_by_type(float_type->type_id);
  ck_assert_msg(reopened_float,
                "reopened Astra::Float definition lookup failed: %s",
                result_message(reopened_float));
  ck_assert_msg(reopened_float.value->definition.kind.has_value(),
                "Astra::Float format metadata missing after reopen");
  ck_assert_str_eq(reopened_float.value->definition.kind->c_str(), "ieee754-binary32");
  ck_assert_uint_eq(reopened_float.value->definition.version, 2U);

  auto reopened_double = reopened_registry.get_definition_by_type(double_type->type_id);
  ck_assert_msg(reopened_double,
                "reopened Astra::Double definition lookup failed: %s",
                result_message(reopened_double));
  ck_assert_msg(reopened_double.value->definition.kind.has_value(),
                "Astra::Double format metadata missing after reopen");
  ck_assert_str_eq(reopened_double.value->definition.kind->c_str(), "ieee754-binary64");
  ck_assert_uint_eq(reopened_double.value->definition.version, 2U);

  ck_assert_msg(reopened.close(), "reopened store close failed");
  cleanup_temp_db(path);
}
END_TEST

START_TEST(test_bootstrap_migrates_astra_float_formats)
{
  const std::string path = make_temp_db_path();
  ObjectID prior_float_id{};
  ObjectID prior_double_id{};
  ObjectID prior_unit_id{};
  ObjectRecord legacy_unit_record;

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "open old store failed");
    ck_assert_msg(store.ensure_schema(), "old store ensure_schema failed");
    SchemaRegistry registry(store);

    TypeDefinition old_float{};
    old_float.type_id = TypeID{0x4153545200000001ULL};
    old_float.name = "Float";
    old_float.namespace_name = "Astra";
    old_float.version = 1;
    auto registered_float = registry.register_definition(old_float);
    ck_assert_msg(registered_float,
                  "register old Astra::Float failed: %s",
                  result_message(registered_float));
    prior_float_id = registered_float.value->ref.id;

    TypeDefinition old_double{};
    old_double.type_id = TypeID{0x4153545200000002ULL};
    old_double.name = "Double";
    old_double.namespace_name = "Astra";
    old_double.version = 1;
    auto registered_double = registry.register_definition(old_double);
    ck_assert_msg(registered_double,
                  "register old Astra::Double failed: %s",
                  result_message(registered_double));
    prior_double_id = registered_double.value->ref.id;

    const auto old_definitions = core_schema_definitions();
    const auto old_unit_definition = std::find_if(old_definitions.begin(), old_definitions.end(),
        [](const TypeDefinition& definition) {
          return definition.type_id == TypeID{0x43414C5000000002ULL};
        });
    const auto old_dimension_definition = std::find_if(old_definitions.begin(), old_definitions.end(),
        [](const TypeDefinition& definition) {
          return definition.type_id == TypeID{0x43414C5000000001ULL};
        });
    ck_assert(old_unit_definition != old_definitions.end());
    ck_assert(old_dimension_definition != old_definitions.end());
    auto registered_dimension = registry.register_definition(*old_dimension_definition);
    ck_assert_msg(registered_dimension, "register old Caliper::Dimension failed: %s",
                  result_message(registered_dimension));
    auto old_dimension = store.create_object(old_dimension_definition->type_id,
        registered_dimension.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{
            { "name", "Length" }, { "symbol", "L" }, { "components", {{"Length", 1}} }
        }));
    ck_assert_msg(old_dimension, "create old Caliper::Dimension record failed: %s",
                  result_message(old_dimension));
    auto registered_unit = registry.register_definition(*old_unit_definition);
    ck_assert_msg(registered_unit, "register old Caliper::Unit failed: %s",
                  result_message(registered_unit));
    prior_unit_id = registered_unit.value->ref.id;
    auto legacy_unit = store.create_object(old_unit_definition->type_id, prior_unit_id,
        nlohmann::json::to_cbor(nlohmann::json{
            { "name", "legacy short unit" }, { "symbol", "lsu" },
            { "dimension_id", old_dimension.value->ref.id.to_hex() },
            { "system", "legacy" }, { "systems", {"legacy"} },
            { "scale", 1.0 }, { "offset", 0.0 }
        }));
    ck_assert_msg(legacy_unit, "create old Unit record failed: %s", result_message(legacy_unit));
    legacy_unit_record = *legacy_unit.value;
    ck_assert_msg(store.close(), "old store close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "reopen old store failed");
    ck_assert_msg(store.ensure_schema(), "reopened old store ensure_schema failed");
    SchemaRegistry registry(store);

    auto migrated = bootstrap_core_schema(registry);
    ck_assert_msg(migrated, "format migration failed: %s", result_message(migrated));
    ck_assert_uint_eq(migrated.value->inserted, 3U);

    auto latest_float = registry.get_latest_definition_by_type(TypeID{0x4153545200000001ULL});
    ck_assert_msg(latest_float,
                  "latest Astra::Float lookup failed: %s",
                  result_message(latest_float));
    ck_assert_uint_eq(latest_float.value->definition.version, 2U);
    ck_assert_str_eq(latest_float.value->definition.kind->c_str(), "ieee754-binary32");
    auto float_chain = registry.list_supersedes_chain(latest_float.value->ref.id);
    ck_assert_msg(float_chain, "Astra::Float supersedes chain lookup failed");
    ck_assert_uint_eq(float_chain.value->size(), 1U);
    ck_assert_msg(float_chain.value->front().prior.ref.id == prior_float_id,
                  "Astra::Float migration does not supersede the v1 definition");

    auto latest_double = registry.get_latest_definition_by_type(TypeID{0x4153545200000002ULL});
    ck_assert_msg(latest_double,
                  "latest Astra::Double lookup failed: %s",
                  result_message(latest_double));
    ck_assert_uint_eq(latest_double.value->definition.version, 2U);
    ck_assert_str_eq(latest_double.value->definition.kind->c_str(), "ieee754-binary64");
    auto double_chain = registry.list_supersedes_chain(latest_double.value->ref.id);
    ck_assert_msg(double_chain, "Astra::Double supersedes chain lookup failed");
    ck_assert_uint_eq(double_chain.value->size(), 1U);
    ck_assert_msg(double_chain.value->front().prior.ref.id == prior_double_id,
                  "Astra::Double migration does not supersede the v1 definition");

    auto latest_unit = registry.get_latest_definition_by_type(TypeID{0x43414C5000000002ULL});
    ck_assert_msg(latest_unit, "latest Caliper::Unit lookup failed: %s", result_message(latest_unit));
    ck_assert_uint_eq(latest_unit.value->definition.version, 2U);
    auto unit_chain = registry.list_supersedes_chain(latest_unit.value->ref.id);
    ck_assert_msg(unit_chain, "Caliper::Unit supersedes chain lookup failed");
    ck_assert_uint_eq(unit_chain.value->size(), 1U);
    ck_assert_msg(unit_chain.value->front().prior.ref.id == prior_unit_id,
                  "Caliper::Unit migration points to the wrong prior definition");
    auto legacy_unit_after = store.get_object(legacy_unit_record.ref);
    ck_assert_msg(legacy_unit_after, "legacy Unit record became unreadable after migration");
    ck_assert_msg(legacy_unit_after.value->payload_cbor == legacy_unit_record.payload_cbor
                      && legacy_unit_after.value->definition_id == legacy_unit_record.definition_id,
                  "legacy Unit record changed during schema migration");

    auto prior_float = registry.get_definition_by_id(prior_float_id);
    ck_assert_msg(prior_float, "prior Float definition lookup failed");
    ck_assert_uint_eq(prior_float.value->definition.version, 1U);
    ck_assert_msg(!prior_float.value->definition.kind.has_value(),
                  "prior Float definition was modified");

    auto prior_double = registry.get_definition_by_id(prior_double_id);
    ck_assert_msg(prior_double, "prior Double definition lookup failed");
    ck_assert_uint_eq(prior_double.value->definition.version, 1U);
    ck_assert_msg(!prior_double.value->definition.kind.has_value(),
                  "prior Double definition was modified");

    auto repeated = bootstrap_core_schema(registry);
    ck_assert_msg(repeated, "repeated format migration failed: %s", result_message(repeated));
    ck_assert_uint_eq(repeated.value->inserted, 0U);
    ck_assert_msg(store.close(), "migrated store close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "reopen migrated store failed");
    ck_assert_msg(store.ensure_schema(), "reopened migrated store ensure_schema failed");
    SchemaRegistry registry(store);
    auto schema_bootstrap = bootstrap_core_schema(registry);
    ck_assert_msg(schema_bootstrap, "reopened schema bootstrap failed: %s",
                  result_message(schema_bootstrap));
    for (const auto& definition : core_schema_definitions()) {
      auto existing = registry.get_definition_by_type(definition.type_id);
      if (existing) continue;
      ck_assert_msg(existing.error->code == ErrorCode::NotFound,
                    "lookup missing core definition failed: %s", result_message(existing));
      auto registered = registry.register_definition(definition);
      ck_assert_msg(registered, "register remaining core definition failed: %s",
                    result_message(registered));
    }
    auto catalog_bootstrap = bootstrap_core_catalog(registry, store);
    ck_assert_msg(catalog_bootstrap, "migrated old store catalog bootstrap failed: %s",
                  result_message(catalog_bootstrap));
    auto loaded_catalog = load_caliper_catalog(registry, store);
    ck_assert_msg(loaded_catalog, "migrated old store catalog did not load: %s",
                  result_message(loaded_catalog));
    auto loaded_legacy = std::find_if(loaded_catalog.value->begin(), loaded_catalog.value->end(),
        [](const CaliperCatalogUnit& unit) { return unit.symbol == "lsu"; });
    ck_assert_msg(loaded_legacy != loaded_catalog.value->end() && loaded_legacy->object_id,
                  "unreferenced v1 Unit was not preserved as an extension after catalog migration");
    auto unchanged_legacy_unit = store.get_object(legacy_unit_record.ref);
    ck_assert_msg(unchanged_legacy_unit
                  && unchanged_legacy_unit.value->payload_cbor == legacy_unit_record.payload_cbor
                  && unchanged_legacy_unit.value->definition_id == legacy_unit_record.definition_id,
                  "catalog migration changed the pre-existing Unit v1 record");
    for (const auto& [type_id, expected_kind, prior_id] : {
             std::tuple{TypeID{0x4153545200000001ULL}, "ieee754-binary32", prior_float_id},
             std::tuple{TypeID{0x4153545200000002ULL}, "ieee754-binary64", prior_double_id}}) {
      auto latest = registry.get_latest_definition_by_type(type_id);
      ck_assert_msg(latest, "reopened migrated definition lookup failed: %s", result_message(latest));
      ck_assert_uint_eq(latest.value->definition.version, 2U);
      ck_assert_str_eq(latest.value->definition.kind->c_str(), expected_kind);
      auto chain = registry.list_supersedes_chain(latest.value->ref.id);
      ck_assert_msg(chain, "reopened migration chain lookup failed");
      ck_assert_uint_eq(chain.value->size(), 1U);
      ck_assert_msg(chain.value->front().prior.ref.id == prior_id,
                    "reopened migration chain points to the wrong prior definition");
    }
    ck_assert_msg(store.close(), "reopened migrated store close failed");
  }

  cleanup_temp_db(path);
}
END_TEST

START_TEST(test_bootstrap_float_format_migration_rejects_invalid_double_atomically)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");
  SchemaRegistry registry(store);

  TypeDefinition old_float{};
  old_float.type_id = TypeID{0x4153545200000001ULL};
  old_float.name = "Float";
  old_float.namespace_name = "Astra";
  old_float.version = 1;
  auto registered_float = registry.register_definition(old_float);
  ck_assert_msg(registered_float, "register old Astra::Float failed");

  TypeDefinition invalid_double{};
  invalid_double.type_id = TypeID{0x4153545200000002ULL};
  invalid_double.name = "Double";
  invalid_double.namespace_name = "Astra";
  invalid_double.version = 2;
  invalid_double.kind = "unsupported-format";
  auto registered_double = registry.register_definition(invalid_double);
  ck_assert_msg(registered_double, "register invalid Astra::Double failed");

  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(!boot, "bootstrap should reject unsupported Astra::Double format metadata");
  ck_assert_msg(boot.error->code == ErrorCode::InvalidArgument,
                "unsupported Astra::Double format should return InvalidArgument");
  ck_assert_str_eq(boot.error->message.c_str(), "unsupported Astra floating format metadata");

  auto latest_float = registry.get_latest_definition_by_type(TypeID{0x4153545200000001ULL});
  ck_assert_msg(latest_float, "latest Astra::Float lookup failed");
  ck_assert_uint_eq(latest_float.value->definition.version, 1U);
  ck_assert_msg(!latest_float.value->definition.kind.has_value(),
                "failed migration partially inserted Astra::Float v2");
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_bootstrap_caliper_units)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  auto catalog = bootstrap_core_catalog(registry, store);
  ck_assert_msg(catalog, "catalog bootstrap failed: %s", result_message(catalog));

  auto listR = registry.list_types();
  ck_assert_msg(listR, "list_types failed: %s", result_message(listR));
  const auto& types = listR.value.value();

  ck_assert_msg(find_type(types, "Caliper", "Unit").has_value(), "Caliper::Unit missing");
  ck_assert_msg(find_type(types, "Caliper", "Dimension").has_value(), "Caliper::Dimension missing");
  ck_assert_msg(find_type(types, "Caliper", "Angle").has_value(), "Caliper::Angle missing");
  ck_assert_msg(find_type(types, "Caliper", "Duration").has_value(), "Caliper::Duration missing");
  ck_assert_msg(find_type(types, "Caliper", "Span").has_value(), "Caliper::Span missing");
  ck_assert_msg(find_type(types, "Caliper", "Range").has_value(), "Caliper::Range missing");
  ck_assert_msg(find_type(types, "Caliper", "Percentage").has_value(), "Caliper::Percentage missing");
  ck_assert_msg(find_type(types, "Caliper", "Ratio").has_value(), "Caliper::Ratio missing");

  auto unit_type = find_type(types, "Caliper", "Unit");
  ck_assert_msg(unit_type.has_value(), "Caliper::Unit missing");

  auto unitsR = store.list_by_type(unit_type->type_id);
  ck_assert_msg(unitsR, "list_by_type failed: %s", result_message(unitsR));
  ck_assert_msg(!unitsR.value->empty(), "no units registered");

  std::set<std::string> symbols;
  const std::vector<std::string> expected_symbols = {
    "m", "kg", "s", "K", "A", "mol", "cd", "°C", "C", "Hz", "N", "Pa", "J", "W",
    "mm", "µm", "nm", "ms", "us", "gal", "fl_oz", "cup", "pt", "qt", "nmi", "kn",
    "sr", "H", "lm", "Bq", "Gy", "Sv", "kat", "Ω"
  };
  for (const auto& rec : unitsR.value.value()) {
    auto payload = nlohmann::json::from_cbor(rec.payload_cbor);
    const auto symbol = payload.value("symbol", "");
    ck_assert_msg(!symbol.empty(), "Caliper unit has no symbol");
    ck_assert_msg(symbols.insert(symbol).second, "duplicate Caliper unit symbol: %s", symbol.c_str());
    if (payload.contains("system")) {
      ck_assert_msg(payload.contains("systems"), "Caliper unit is missing system tags: %s", symbol.c_str());
      ck_assert_msg(payload.at("systems").is_array(), "Caliper systems must be an array: %s", symbol.c_str());
      ck_assert_msg(!payload.at("systems").empty(), "Caliper systems must not be empty: %s", symbol.c_str());
    }
  }
  for (const auto& symbol : expected_symbols) {
    ck_assert_msg(symbols.count(symbol) == 1, "expected Caliper unit missing: %s", symbol.c_str());
  }

  bool found_meter = false;
  for (const auto& rec : unitsR.value.value()) {
    if (payload_has_symbol(rec, "m")) {
      found_meter = true;
      break;
    }
  }
  ck_assert_msg(found_meter, "meter unit missing");

  auto catalog_type = find_type(types, "Caliper", "Catalog");
  ck_assert_msg(catalog_type.has_value(), "Caliper::Catalog missing");
  auto catalogsR = store.list_by_type(catalog_type->type_id);
  ck_assert_msg(catalogsR, "list catalog objects failed: %s", result_message(catalogsR));
  ck_assert_int_eq((int)catalogsR.value->size(), 1);
  auto saved_catalog = nlohmann::json::from_cbor(catalogsR.value->front().payload_cbor);
  ck_assert_int_eq(saved_catalog.value("catalog_version", 0), 2);
  const auto temperature_dimension = std::find_if(
      saved_catalog.at("dimensions").begin(), saved_catalog.at("dimensions").end(),
      [](const nlohmann::json& dimension) { return dimension.value("name", "") == "Temperature"; });
  ck_assert(temperature_dimension != saved_catalog.at("dimensions").end());
  ck_assert_int_eq((int)temperature_dimension->at("constraints").size(), 1);
  ck_assert_str_eq(temperature_dimension->at("constraints").front().at("value").get<std::string>().c_str(), "0");
  ck_assert_int_eq((int)temperature_dimension->at("exact_transforms").size(), 1);
  ck_assert_int_eq(temperature_dimension->at("exact_transforms").front()
                       .at("scale").at("numerator").get<int>(), 5);
  ck_assert_int_eq(temperature_dimension->at("exact_transforms").front()
                       .at("scale").at("denominator").get<int>(), 9);
  ck_assert_int_eq(temperature_dimension->at("exact_transforms").front()
                       .at("offset").at("numerator").get<int>(), 45967);
  ck_assert_int_eq(temperature_dimension->at("exact_transforms").front()
                       .at("offset").at("denominator").get<int>(), 180);
  ck_assert_int_eq((int)saved_catalog.at("units").size(), (int)symbols.size());
  const std::map<std::string, std::pair<std::string, std::string>> si_core = {
    { "s", { "second", "Time" } }, { "m", { "meter", "Length" } },
    { "kg", { "kilogram", "Mass" } }, { "A", { "ampere", "ElectricCurrent" } },
    { "K", { "kelvin", "Temperature" } }, { "mol", { "mole", "AmountOfSubstance" } },
    { "cd", { "candela", "LuminousIntensity" } }, { "Hz", { "hertz", "Frequency" } },
    { "N", { "newton", "Force" } }, { "Pa", { "pascal", "Pressure" } },
    { "J", { "joule", "Energy" } }, { "W", { "watt", "Power" } },
    { "C", { "coulomb", "ElectricCharge" } }, { "V", { "volt", "Voltage" } },
    { "F", { "farad", "Capacitance" } }, { "Ω", { "ohm", "Resistance" } },
    { "S", { "siemens", "Conductance" } }, { "Wb", { "weber", "MagneticFlux" } },
    { "T", { "tesla", "MagneticFluxDensity" } }, { "H", { "henry", "Inductance" } },
    { "lm", { "lumen", "LuminousIntensity" } }, { "lx", { "lux", "Illuminance" } },
    { "Bq", { "becquerel", "Frequency" } }, { "Gy", { "gray", "AbsorbedDose" } },
    { "Sv", { "sievert", "EquivalentDose" } }, { "kat", { "katal", "CatalyticActivity" } },
    { "rad", { "radian", "Angle" } }, { "sr", { "steradian", "Dimensionless" } },
    { "°C", { "celsius", "Temperature" } }
  };
  ck_assert_int_eq((int)si_core.size(), 29);
  for (const auto& [symbol, expected] : si_core) {
    bool found = false;
    for (const auto& unit : saved_catalog.at("units")) {
      if (unit.value("symbol", "") != symbol) continue;
      found = true;
      ck_assert_msg(unit.value("name", "") == expected.first,
                    "unexpected SI unit name for %s", symbol.c_str());
      ck_assert_msg(unit.value("dimension", "") == expected.second,
                    "unexpected SI unit dimension for %s", symbol.c_str());
      ck_assert_double_eq(unit.value("scale", 0.0), 1.0);
      ck_assert_double_eq(unit.value("offset", -1.0), symbol == "°C" ? 273.15 : 0.0);
      break;
    }
    ck_assert_msg(found, "SI core unit missing from catalog v2: %s", symbol.c_str());
  }
  const std::map<std::string, nlohmann::json> expected_components = {
    { "Inductance", {{"Mass", 1}, {"Length", 2}, {"Time", -2}, {"ElectricCurrent", -2}} },
    { "CatalyticActivity", {{"AmountOfSubstance", 1}, {"Time", -1}} },
    { "AbsorbedDose", {{"Length", 2}, {"Time", -2}} },
    { "EquivalentDose", {{"Length", 2}, {"Time", -2}} }
  };
  for (const auto& [name, components] : expected_components) {
    bool found = false;
    for (const auto& dimension : saved_catalog.at("dimensions")) {
      if (dimension.value("name", "") != name) continue;
      ck_assert_msg(dimension.at("components") == components,
                    "incorrect SI dimension components for %s", name.c_str());
      found = true;
      break;
    }
    ck_assert_msg(found, "SI dimension missing from catalog v2: %s", name.c_str());
  }
  ck_assert_int_eq((int)saved_catalog.at("prefixes").size(), 24);
  const std::map<std::string, std::pair<std::string, int>> prefix_manifest = {
    { "Q", { "quetta", 30 } }, { "R", { "ronna", 27 } }, { "Y", { "yotta", 24 } },
    { "Z", { "zetta", 21 } }, { "E", { "exa", 18 } }, { "P", { "peta", 15 } },
    { "T", { "tera", 12 } }, { "G", { "giga", 9 } }, { "M", { "mega", 6 } },
    { "k", { "kilo", 3 } }, { "h", { "hecto", 2 } }, { "da", { "deca", 1 } },
    { "d", { "deci", -1 } }, { "c", { "centi", -2 } }, { "m", { "milli", -3 } },
    { "µ", { "micro", -6 } }, { "n", { "nano", -9 } }, { "p", { "pico", -12 } },
    { "f", { "femto", -15 } }, { "a", { "atto", -18 } }, { "z", { "zepto", -21 } },
    { "y", { "yocto", -24 } }, { "r", { "ronto", -27 } }, { "q", { "quecto", -30 } },
  };
  for (const auto& prefix : saved_catalog.at("prefixes")) {
    const auto symbol = prefix.at("symbol").get<std::string>();
    ck_assert_msg(prefix_manifest.contains(symbol), "unexpected SI prefix: %s", symbol.c_str());
    ck_assert_str_eq(prefix.at("name").get<std::string>().c_str(), prefix_manifest.at(symbol).first.c_str());
    ck_assert_int_eq(prefix.at("exponent").get<int>(), prefix_manifest.at(symbol).second);
  }
  using CustomaryExpectation = std::tuple<std::string, std::string, std::string, double, double, std::string>;
  const auto hb44 = [](const std::string& category) {
    return "NIST HB44 (2026), Appendix C §4, " + category;
  };
  const auto force_source = "NIST SP 811, Appendix B.9, force";
  const auto power_source = "NIST SP 811, Appendix B.9, power";
  const std::map<std::string, CustomaryExpectation> customary_manifest = {
    { "in", { "inch", "Length", "m", 0.0254, 0.0, hb44("Units of Length") } },
    { "mil", { "mil", "Length", "m", 0.0000254, 0.0, hb44("Units of Length – Thickness Measurement") } },
    { "ft", { "foot", "Length", "m", 0.3048, 0.0, hb44("Units of Length") } },
    { "yd", { "yard", "Length", "m", 0.9144, 0.0, hb44("Units of Length") } },
    { "mi", { "mile", "Length", "m", 1609.344, 0.0, hb44("Units of Length") } },
    { "lb", { "pound", "Mass", "kg", 0.45359237, 0.0, hb44("Units of Mass") } },
    { "oz", { "ounce", "Mass", "kg", 0.028349523125, 0.0, hb44("Units of Mass") } },
    { "dr", { "avoirdupois_dram", "Mass", "kg", 0.0017718451953125, 0.0, hb44("Units of Mass") } },
    { "cwt", { "short_hundredweight", "Mass", "kg", 45.359237, 0.0, hb44("Units of Mass") } },
    { "tn", { "short_ton", "Mass", "kg", 907.18474, 0.0, hb44("Units of Mass") } },
    { "°F", { "fahrenheit", "Temperature", "K", 0.5555555555555556, 255.3722222222222,
              hb44("Units of Temperature") } },
    { "ft^2", { "square_foot", "Area", "m^2", 0.09290304, 0.0, hb44("Units of Area") } },
    { "in^2", { "square_inch", "Area", "m^2", 0.00064516, 0.0, hb44("Units of Area") } },
    { "yd^2", { "square_yard", "Area", "m^2", 0.83612736, 0.0, hb44("Units of Area") } },
    { "mi^2", { "square_mile", "Area", "m^2", 2589988.110336, 0.0, hb44("Units of Area") } },
    { "ac", { "acre", "Area", "m^2", 4046.8564224, 0.0, hb44("Units of Area") } },
    { "ft^3", { "cubic_foot", "Volume", "m^3", 0.028316846592, 0.0, hb44("Units of Volume") } },
    { "in^3", { "cubic_inch", "Volume", "m^3", 0.000016387064, 0.0, hb44("Units of Volume") } },
    { "yd^3", { "cubic_yard", "Volume", "m^3", 0.764554857984, 0.0, hb44("Units of Volume") } },
    { "gal", { "us_gallon", "Volume", "m^3", 0.003785411784, 0.0, hb44("Units of Liquid Capacity") } },
    { "minim", { "us_minim", "Volume", "m^3", 0.000000061611519921875, 0.0,
                 "Derived as 1/60 U.S. fluid dram; " + hb44("Units of Liquid Capacity") } },
    { "fl_dr", { "us_fluid_dram", "Volume", "m^3", 0.0000036966911953125, 0.0,
                 "Derived as 1/8 U.S. fluid ounce; " + hb44("Units of Liquid Capacity") } },
    { "fl_oz", { "us_fluid_ounce", "Volume", "m^3", 0.0000295735295625, 0.0,
                 hb44("Units of Liquid Capacity") } },
    { "gi", { "us_gill", "Volume", "m^3", 0.00011829411825, 0.0,
              "Derived as 1/32 U.S. gallon; " + hb44("Units of Liquid Capacity") } },
    { "cup", { "us_cup", "Volume", "m^3", 0.0002365882365, 0.0,
               "Derived as 8 U.S. fluid ounces; " + hb44("Units of Liquid Capacity") } },
    { "pt", { "us_pint", "Volume", "m^3", 0.000473176473, 0.0, hb44("Units of Liquid Capacity") } },
    { "qt", { "us_quart", "Volume", "m^3", 0.000946352946, 0.0, hb44("Units of Liquid Capacity") } },
    { "dry_pt", { "us_dry_pint", "Volume", "m^3", 0.0005506104713575, 0.0,
                  hb44("Units of Dry Capacity") } },
    { "dry_qt", { "us_dry_quart", "Volume", "m^3", 0.001101220942715, 0.0,
                  hb44("Units of Dry Capacity") } },
    { "pk", { "us_peck", "Volume", "m^3", 0.00880976754172, 0.0, hb44("Units of Dry Capacity") } },
    { "bu", { "us_bushel", "Volume", "m^3", 0.03523907016688, 0.0, hb44("Units of Dry Capacity") } },
    { "mph", { "mile_per_hour", "Velocity", "m/s", 0.44704, 0.0, hb44("Units of Velocity") } },
    { "nmi", { "nautical_mile", "Length", "m", 1852.0, 0.0, "NIST SP 811, Appendix B.9, nautical mile" } },
    { "kn", { "knot", "Velocity", "m/s", 0.5144444444444445, 0.0, "NIST SP 811, Appendix B.9, knot" } },
    { "lbf", { "pound_force", "Force", "N", 4.4482216152605, 0.0, force_source } },
    { "ozf", { "ounce_force", "Force", "N", 0.27801385095378125, 0.0, force_source } },
    { "kip", { "kip", "Force", "N", 4448.2216152605, 0.0, force_source } },
    { "tonf", { "short_ton_force", "Force", "N", 8896.443230521, 0.0, force_source } },
    { "psi", { "psi", "Pressure", "Pa", 6894.757293168, 0.0, hb44("Units of Pressure") } },
    { "mmHg", { "millimeter_mercury", "Pressure", "Pa", 133.3224, 0.0, hb44("Units of Pressure") } },
    { "inH2O", { "inch_water", "Pressure", "Pa", 249.082, 0.0, hb44("Units of Pressure") } },
    { "ft·lbf/s", { "foot_pound_force_per_second", "Power", "W", 1.3558179483314004, 0.0,
                    power_source } },
    { "hp", { "horsepower", "Power", "W", 745.69987158227022, 0.0,
              "NIST SP 811, Appendix B.9, power (mechanical horsepower)" } },
  };
  std::set<std::string> actual_customary_symbols;
  for (const auto& [symbol, expected] : customary_manifest) {
    bool found = false;
    for (const auto& unit : saved_catalog.at("units")) {
      if (unit.value("symbol", "") != symbol) continue;
      found = true;
      actual_customary_symbols.insert(symbol);
      const auto name = unit.at("name").get<std::string>();
      const auto dimension = unit.at("dimension").get<std::string>();
      const auto base_symbol = unit.at("base_symbol").get<std::string>();
      const auto source = unit.at("source").get<std::string>();
      ck_assert_str_eq(name.c_str(), std::get<0>(expected).c_str());
      ck_assert_str_eq(dimension.c_str(), std::get<1>(expected).c_str());
      ck_assert_str_eq(base_symbol.c_str(), std::get<2>(expected).c_str());
      ck_assert(std::abs(unit.at("scale").get<double>() - std::get<3>(expected)) < 1e-15);
      ck_assert(std::abs(unit.at("offset").get<double>() - std::get<4>(expected)) < 1e-15);
      ck_assert_str_eq(source.c_str(), std::get<5>(expected).c_str());
      break;
    }
    ck_assert_msg(found, "customary manifest unit missing: %s", symbol.c_str());
  }
  std::set<std::string> expected_customary_symbols;
  for (const auto& [symbol, expected] : customary_manifest) {
    (void)expected;
    expected_customary_symbols.insert(symbol);
  }
  for (const auto& unit : saved_catalog.at("units")) {
    bool non_si = false;
    for (const auto& system : unit.at("systems")) {
      const auto name = system.get<std::string>();
      non_si = non_si || name == "imperial" || name == "us_customary" || name == "international";
    }
    if (non_si) {
      const auto symbol = unit.at("symbol").get<std::string>();
      actual_customary_symbols.insert(symbol);
      ck_assert_msg(customary_manifest.contains(symbol), "unlisted customary unit present: %s", symbol.c_str());
      ck_assert_msg(unit.contains("source"), "customary source missing for: %s", symbol.c_str());
    }
  }
  ck_assert_msg(actual_customary_symbols == expected_customary_symbols,
                "customary unit manifest differs from the exact selected set");
  const auto saved_catalog_payload = catalogsR.value->front().payload_cbor;
  auto repeat = bootstrap_core_catalog(registry, store);
  ck_assert_msg(repeat, "repeated catalog bootstrap failed: %s", result_message(repeat));
  auto catalogsAfterRepeatR = store.list_by_type(catalog_type->type_id);
  ck_assert_msg(catalogsAfterRepeatR, "list repeated catalog objects failed: %s",
                result_message(catalogsAfterRepeatR));
  ck_assert_int_eq((int)catalogsAfterRepeatR.value->size(), 1);
  ck_assert_msg(catalogsAfterRepeatR.value->front().payload_cbor == saved_catalog_payload,
                "Caliper base catalog serialization changed after reload");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_caliper_quantity_unit_attachment_roundtrip)
{
  const std::string path = make_temp_db_path();
  SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);
  auto boot = bootstrap_core_schema(registry);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));
  auto catalog = bootstrap_core_catalog(registry, store);
  ck_assert_msg(catalog, "catalog bootstrap failed: %s", result_message(catalog));

  auto typesR = registry.list_types();
  ck_assert_msg(typesR, "list types failed: %s", result_message(typesR));
  const auto& types = typesR.value.value();
  const auto angle = find_type(types, "Caliper", "Angle");
  const auto duration = find_type(types, "Caliper", "Duration");
  const auto span = find_type(types, "Caliper", "Span");
  const auto range = find_type(types, "Caliper", "Range");
  const auto percentage = find_type(types, "Caliper", "Percentage");
  const auto ratio = find_type(types, "Caliper", "Ratio");
  const auto unit_type = find_type(types, "Caliper", "Unit");
  const auto dimension_type = find_type(types, "Caliper", "Dimension");
  ck_assert_msg(angle && duration && span && range && percentage && ratio && unit_type && dimension_type,
                "Caliper quantity schemas missing");

  auto unitsR = store.list_by_type(unit_type->type_id);
  ck_assert_msg(unitsR, "list units failed: %s", result_message(unitsR));
  std::map<std::string, ObjectID> unit_ids;
  for (const auto& unit : unitsR.value.value()) {
    const auto payload = nlohmann::json::from_cbor(unit.payload_cbor);
    unit_ids[payload.at("symbol").get<std::string>()] = unit.ref.id;
  }
  ck_assert_msg(unit_ids.contains("rad") && unit_ids.contains("s") && unit_ids.contains("m") &&
                unit_ids.contains("%") && unit_ids.contains("1"),
                "required Caliper units missing");

  CaliperValueRegistry values(registry, store);
  auto angle_value = values.create(angle->type_id, {{0x01, 0x02}}, unit_ids.at("rad"));
  auto duration_value = values.create(duration->type_id, {{0x03}}, unit_ids.at("s"));
  auto span_value = values.create(span->type_id, {{0x04}}, unit_ids.at("m"));
  auto range_value = values.create(range->type_id, {{0x05}, {0x06}}, unit_ids.at("m"));
  auto percentage_value = values.create(percentage->type_id, {{0x07}}, unit_ids.at("%"));
  auto ratio_value = values.create(ratio->type_id, {{0x08}}, unit_ids.at("1"));
  ck_assert_msg(angle_value, "create Angle failed: %s", result_message(angle_value));
  ck_assert_msg(duration_value, "create Duration failed: %s", result_message(duration_value));
  ck_assert_msg(span_value, "create Span failed: %s", result_message(span_value));
  ck_assert_msg(range_value, "create Range failed: %s", result_message(range_value));
  ck_assert_msg(percentage_value, "create Percentage failed: %s", result_message(percentage_value));
  ck_assert_msg(ratio_value, "create Ratio failed: %s", result_message(ratio_value));
  const std::vector<CaliperQuantityValue> created_values = {
      angle_value.value.value(), duration_value.value.value(), span_value.value.value(),
      range_value.value.value(), percentage_value.value.value(), ratio_value.value.value()};

  const auto angle_count_before_rejections = store.list_by_type(angle->type_id).value->size();
  auto mismatch = values.create(angle->type_id, {{0x09}}, unit_ids.at("m"));
  ck_assert_msg(!mismatch, "dimension-mismatched Angle unit unexpectedly accepted");
  ck_assert_msg(mismatch.error->code == ErrorCode::InvalidArgument,
                "dimension mismatch should return InvalidArgument");
  auto missing_unit = values.create(angle->type_id, {{0x09}}, ObjectID::random());
  ck_assert_msg(!missing_unit, "missing unit reference unexpectedly accepted");
  ck_assert_msg(missing_unit.error->code == ErrorCode::InvalidArgument,
                "missing unit should return InvalidArgument");
  auto dimensionsR = store.list_by_type(dimension_type->type_id);
  ck_assert_msg(dimensionsR, "list dimensions failed: %s", result_message(dimensionsR));
  auto wrong_type_ref = values.create(angle->type_id, {{0x09}}, dimensionsR.value->front().ref.id);
  ck_assert_msg(!wrong_type_ref, "non-Unit object reference unexpectedly accepted");
  ck_assert_msg(wrong_type_ref.error->code == ErrorCode::InvalidArgument,
                "wrong unit object type should return InvalidArgument");
  auto dimension_def = registry.get_definition_by_type(dimension_type->type_id);
  auto unit_def = registry.get_definition_by_type(unit_type->type_id);
  ck_assert_msg(dimension_def && unit_def, "Caliper Unit/Dimension definitions missing");
  nlohmann::json forged_dimension_payload{{"name", "Angle"}, {"symbol", "Ang"},
                                           {"components", {{"Length", 1}}}};
  auto forged_dimension = store.create_object(dimension_type->type_id, dimension_def.value->ref.id,
                                               nlohmann::json::to_cbor(forged_dimension_payload));
  ck_assert_msg(forged_dimension, "create forged dimension fixture failed: %s",
                result_message(forged_dimension));
  nlohmann::json forged_unit_payload{{"name", "forged radian"}, {"symbol", "frad"},
                                      {"dimension_id", forged_dimension.value->ref.id.to_hex()}};
  auto forged_unit = store.create_object(unit_type->type_id, unit_def.value->ref.id,
                                          nlohmann::json::to_cbor(forged_unit_payload));
  ck_assert_msg(forged_unit, "create forged unit fixture failed: %s", result_message(forged_unit));
  auto forged_mismatch = values.create(angle->type_id, {{0x09}}, forged_unit.value->ref.id);
  ck_assert_msg(!forged_mismatch, "Angle accepted a same-name dimension with Length components");
  ck_assert_msg(forged_mismatch.error->code == ErrorCode::InvalidArgument,
                "forged dimension should return InvalidArgument on create");
  ck_assert_uint_eq(store.list_by_type(angle->type_id).value->size(), angle_count_before_rejections);

  auto angle_def = registry.get_definition_by_type(angle->type_id);
  ck_assert_msg(angle_def, "Angle definition lookup failed: %s", result_message(angle_def));
  nlohmann::json legacy_payload;
  legacy_payload["value"] = std::vector<std::uint8_t>{0x0A};
  auto legacy = store.create_object(angle->type_id, angle_def.value->ref.id,
                                    nlohmann::json::to_cbor(legacy_payload));
  ck_assert_msg(legacy, "create unitless legacy quantity failed: %s", result_message(legacy));
  auto legacy_value = values.get(legacy.value->ref);
  ck_assert_msg(legacy_value, "unitless legacy quantity did not decode: %s", result_message(legacy_value));
  ck_assert_msg(!legacy_value.value->unit_id.has_value(), "unitless legacy value gained a unit");
  ck_assert_uint_eq(legacy_value.value->components.size(), 1U);
  ck_assert_uint_eq(legacy_value.value->components[0].at(0), 0x0A);

  nlohmann::json corrupt_payload;
  corrupt_payload["value"] = std::vector<std::uint8_t>{0x0B};
  corrupt_payload["unit_id"] = ObjectID::random().to_hex();
  auto corrupt = store.create_object(angle->type_id, angle_def.value->ref.id,
                                     nlohmann::json::to_cbor(corrupt_payload));
  ck_assert_msg(corrupt, "create corrupt quantity fixture failed: %s", result_message(corrupt));
  auto corrupt_value = values.get(corrupt.value->ref);
  ck_assert_msg(!corrupt_value, "malformed persisted unit reference unexpectedly decoded");
  ck_assert_msg(corrupt_value.error->code == ErrorCode::CorruptData,
                "malformed persisted unit reference should return CorruptData");

  nlohmann::json forged_corrupt_payload;
  forged_corrupt_payload["value"] = std::vector<std::uint8_t>{0x0C};
  forged_corrupt_payload["unit_id"] = forged_unit.value->ref.id.to_hex();
  auto forged_corrupt = store.create_object(angle->type_id, angle_def.value->ref.id,
                                             nlohmann::json::to_cbor(forged_corrupt_payload));
  ck_assert_msg(forged_corrupt, "create forged persisted quantity failed: %s",
                result_message(forged_corrupt));
  auto forged_corrupt_value = values.get(forged_corrupt.value->ref);
  ck_assert_msg(!forged_corrupt_value, "persisted Angle accepted forged dimension components");
  ck_assert_msg(forged_corrupt_value.error->code == ErrorCode::CorruptData,
                "forged persisted dimension should return CorruptData");

  ck_assert_msg(store.close(), "close before reopen failed");
  ck_assert_msg(store.open(), "reopen failed");
  SchemaRegistry reopened_registry(store);
  CaliperValueRegistry reopened_values(reopened_registry, store);
  for (const auto& created : created_values) {
    auto reopened = reopened_values.get(created.ref);
    ck_assert_msg(reopened, "quantity failed persistence round trip: %s", result_message(reopened));
    ck_assert_uint_eq(reopened.value->type.v, created.type.v);
    ck_assert_msg(reopened.value->unit_id == created.unit_id, "unit reference changed on reopen");
    ck_assert_uint_eq(reopened.value->components.size(), created.components.size());
    for (std::size_t i = 0; i < created.components.size(); ++i) {
      ck_assert_msg(reopened.value->components[i] == created.components[i],
                    "quantity component changed on reopen");
    }
  }

  ck_assert_msg(store.close(), "close failed");
  cleanup_temp_db(path);
}
END_TEST

START_TEST(test_compose_caliper_catalog_extension_precedence)
{
  const std::vector<CaliperCatalogUnit> base = {
    { "meter", "m", "Length", { "SI" }, std::nullopt, std::nullopt, std::nullopt, false },
    { "second", "s", "Time", { "SI" }, std::nullopt, std::nullopt, std::nullopt, false },
  };
  const std::vector<CaliperCatalogUnit> extension = {
    { "custom_meter", "m", "Length", { "user" }, std::string("m"), 1.0, 0.0, true },
    { "furlong", "fur", "Length", { "user" }, std::string("m"), 201.168, 0.0, false },
  };

  auto composed = compose_caliper_catalog(base, extension);
  ck_assert_msg(composed, "valid extension composition failed: %s", result_message(composed));
  ck_assert_int_eq((int)composed.value->size(), 3);
  ck_assert_str_eq(composed.value->at(0).symbol.c_str(), "fur");
  ck_assert_str_eq(composed.value->at(1).name.c_str(), "custom_meter");
  ck_assert(composed.value->at(1).override_base);
  ck_assert_str_eq(composed.value->at(2).symbol.c_str(), "s");

  auto implicit_override = extension;
  implicit_override.front().override_base = false;
  auto rejected_implicit = compose_caliper_catalog(base, implicit_override);
  ck_assert(!rejected_implicit);

  auto dimension_change = extension;
  dimension_change.front().dimension = "Time";
  auto rejected_dimension_change = compose_caliper_catalog(base, dimension_change);
  ck_assert(!rejected_dimension_change);

  auto duplicate_symbol = extension;
  duplicate_symbol.push_back(extension.back());
  auto rejected_duplicate = compose_caliper_catalog(base, duplicate_symbol);
  ck_assert(!rejected_duplicate);

  auto duplicate_base = base;
  duplicate_base.push_back(base.front());
  auto rejected_base = compose_caliper_catalog(duplicate_base, {});
  ck_assert(!rejected_base);
}
END_TEST

START_TEST(test_caliper_runtime_conversion_and_catalog_loading)
{
  const auto path = make_temp_db_path();
  SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");
  SchemaRegistry registry(store);
  auto schema = bootstrap_core_schema(registry);
  ck_assert_msg(schema, "schema bootstrap failed: %s", result_message(schema));
  auto bootstrap = bootstrap_core_catalog(registry, store);
  ck_assert_msg(bootstrap, "catalog bootstrap failed: %s", result_message(bootstrap));

  auto catalog = load_caliper_catalog(registry, store);
  ck_assert_msg(catalog, "catalog load failed: %s", result_message(catalog));
  ck_assert_msg(!catalog.value->empty(), "loaded Caliper catalog is empty");
  ck_assert_msg(std::is_sorted(catalog.value->begin(), catalog.value->end(),
      [](const CaliperCatalogUnit& left, const CaliperCatalogUnit& right) {
        return left.symbol < right.symbol;
      }), "persisted Caliper catalog is not ordered by symbol");
  auto find_unit = [&](const std::string& symbol) -> const CaliperCatalogUnit* {
    for (const auto& unit : *catalog.value) if (unit.symbol == symbol) return &unit;
    return nullptr;
  };
  const auto* canonical_meter = find_unit("m");
  ck_assert_msg(canonical_meter != nullptr && canonical_meter->object_id,
                "persisted base meter reference is missing");
  const auto canonical_meter_id = *canonical_meter->object_id;
  auto canonical_meter_record = store.get_latest(canonical_meter_id);
  ck_assert_msg(canonical_meter_record, "persisted meter record is missing");
  const auto meter_payload = nlohmann::json::from_cbor(canonical_meter_record.value->payload_cbor);
  const auto length_dimension_id = ObjectID::from_hex(meter_payload.at("dimension_id").get<std::string>());
  auto original_dimension_record = store.get_latest(length_dimension_id);
  ck_assert_msg(original_dimension_record, "persisted Length dimension is missing");
  auto legacy_unit_definition = registry.get_definition_by_type(TypeID{0x43414C5000000002ULL});
  ck_assert_msg(legacy_unit_definition, "legacy Unit v1 definition is missing");
  const auto legacy_unit_payload = nlohmann::json::to_cbor(nlohmann::json{
      { "name", "legacy short unit" }, { "symbol", "lsu" },
      { "dimension_id", nlohmann::json::from_cbor(canonical_meter_record.value->payload_cbor)
                            .at("dimension_id") },
      { "system", "legacy" }, { "systems", nlohmann::json::array({ "legacy" }) },
      { "base_unit_id", canonical_meter_id.to_hex() }, { "scale", 2.0 }, { "offset", 0.0 }
  });
  auto legacy_unit_record = store.create_object(TypeID{0x43414C5000000002ULL},
      legacy_unit_definition.value->ref.id, legacy_unit_payload);
  ck_assert_msg(legacy_unit_record, "legacy Unit v1 extension creation failed: %s",
                result_message(legacy_unit_record));
  const auto* canonical_kelvin = find_unit("K");
  ck_assert_msg(canonical_kelvin && canonical_kelvin->object_id,
                "canonical Kelvin identity is missing for legacy alias fixtures");
  auto kelvin_record = store.get_latest(*canonical_kelvin->object_id);
  ck_assert_msg(kelvin_record, "persisted Kelvin record is missing");
  const auto temperature_dimension_id = nlohmann::json::from_cbor(kelvin_record.value->payload_cbor)
                                           .at("dimension_id").get<std::string>();
  for (const auto& [name, symbol] : {
           std::pair<std::string, std::string>{"celsius", "C"}, {"fahrenheit", "F"}}) {
    const auto payload = nlohmann::json::to_cbor(nlohmann::json{
        { "name", name }, { "symbol", symbol }, { "dimension_id", temperature_dimension_id },
        { "system", "legacy" }, { "systems", nlohmann::json::array({"legacy"}) },
        { "scale", 1.0 }, { "offset", 0.0 }
    });
    auto alias = store.create_object(TypeID{0x43414C5000000002ULL},
        legacy_unit_definition.value->ref.id, payload);
    ck_assert_msg(alias, "legacy %s alias fixture creation failed: %s", name.c_str(),
                  result_message(alias));
  }
  auto catalog_with_legacy_unit = load_caliper_catalog(registry, store);
  ck_assert_msg(catalog_with_legacy_unit, "unreferenced v1 Unit record did not load: %s",
                result_message(catalog_with_legacy_unit));
  auto attempted_v2_retired_alias = write_caliper_extension_unit(registry, store, CaliperCatalogUnit{
      "celsius", "C", "Temperature", { "user" }, std::nullopt, 1.0, 0.0, false});
  ck_assert(!attempted_v2_retired_alias);
  auto legacy_unit_conversion = convert_caliper_value(*catalog_with_legacy_unit.value,
      "1", "lsu", "m");
  ck_assert_msg(legacy_unit_conversion, "unreferenced v1 Unit was not usable: %s",
                result_message(legacy_unit_conversion));
  ck_assert(std::abs(legacy_unit_conversion.value->value - 2.0) < 1e-15);
  auto missing_direct_parent_catalog = *catalog.value;
  auto missing_direct_parent = missing_direct_parent_catalog.front();
  missing_direct_parent.name = "missing direct parent fixture";
  missing_direct_parent.symbol = "mdpf";
  missing_direct_parent.base_symbol.reset();
  missing_direct_parent.base_object_id = ObjectID::from_hex("00000000000000000000000000000001");
  missing_direct_parent_catalog.push_back(missing_direct_parent);
  auto missing_direct_parent_result = convert_caliper_value(
      missing_direct_parent_catalog, "1", "mdpf", "m");
  ck_assert_msg(!missing_direct_parent_result,
                "missing direct parent should return an error, not convert");
  const auto* quettameter = find_unit("Qm");
  ck_assert_msg(quettameter != nullptr, "algorithmically composed quettameter missing");
  ck_assert_str_eq(quettameter->name.c_str(), "quettameter");
  ck_assert_str_eq(quettameter->base_symbol->c_str(), "m");
  ck_assert(std::abs(quettameter->scale.value() - 1e30) / 1e30 < 1e-15);
  const auto* milligram = find_unit("mg");
  ck_assert_msg(milligram != nullptr, "algorithmically composed milligram missing");
  ck_assert_str_eq(milligram->base_symbol->c_str(), "g");
  ck_assert(std::abs(milligram->scale.value() - 1e-3) < 1e-18);
  ck_assert(find_unit("kg") != nullptr);
  ck_assert(find_unit("kkg") == nullptr);
  auto generated_prefix_conversion = convert_caliper_value(*catalog.value, "1000", "mg", "g");
  ck_assert_msg(generated_prefix_conversion, "generated prefix conversion failed: %s",
                result_message(generated_prefix_conversion));
  ck_assert(std::abs(generated_prefix_conversion.value->value - 1.0) < 1e-15);
  const auto* prefixed_celsius = find_unit("m°C");
  ck_assert_msg(prefixed_celsius != nullptr, "prefixed Celsius unit missing");
  ck_assert_double_eq(prefixed_celsius->offset.value(), 0.0);

  const auto original_meter_id = canonical_meter_id;
  auto original_meter_record = store.get_latest(original_meter_id);
  ck_assert_msg(original_meter_record, "could not snapshot immutable base meter");
  auto catalog_records = store.list_by_type(TypeID{0x43414C5000000009ULL});
  ck_assert_msg(catalog_records, "could not snapshot immutable base catalog");
  const auto active_catalog = std::find_if(catalog_records.value->begin(), catalog_records.value->end(),
      [](const ObjectRecord& record) { return record.ref.id.to_hex() == "43414c495045522d424153452d303032"; });
  ck_assert_msg(active_catalog != catalog_records.value->end(), "active catalog v2 record is missing");
  const auto original_catalog_record = *active_catalog;
  const auto original_catalog_payload = active_catalog->payload_cbor;
  std::vector<ObjectRecord> immutable_base_records;
  const auto catalog_snapshot_json = nlohmann::json::from_cbor(original_catalog_payload);
  for (const auto& entry : catalog_snapshot_json.at("dimensions")) {
    auto record = store.get_latest(ObjectID::from_hex(entry.at("dimension_id").get<std::string>()));
    ck_assert_msg(record, "could not snapshot a catalog Dimension");
    immutable_base_records.push_back(*record.value);
  }
  for (const auto& entry : catalog_snapshot_json.at("units")) {
    auto record = store.get_latest(ObjectID::from_hex(entry.at("unit_id").get<std::string>()));
    ck_assert_msg(record, "could not snapshot a catalog Unit");
    immutable_base_records.push_back(*record.value);
  }
  auto base_snapshot_unchanged = [&]() {
    auto catalog_after = store.get_latest(original_catalog_record.ref.id);
    if (!catalog_after || catalog_after.value->ref.ver.v != original_catalog_record.ref.ver.v
        || catalog_after.value->definition_id != original_catalog_record.definition_id
        || catalog_after.value->payload_cbor != original_catalog_record.payload_cbor) return false;
    for (const auto& snapshot : immutable_base_records) {
      auto unchanged = store.get_latest(snapshot.ref.id);
      if (!unchanged || unchanged.value->ref.ver.v != snapshot.ref.ver.v
          || unchanged.value->definition_id != snapshot.definition_id
          || unchanged.value->payload_cbor != snapshot.payload_cbor) return false;
    }
    return true;
  };

  CaliperCatalogUnit furlong{
    "furlong", "fur", "Length", { "user" }, std::string("m"), 201.168, 0.0, false
  };
  furlong.base_object_id = canonical_meter_id;
  auto furlong_record = write_caliper_extension_unit(registry, store, furlong);
  ck_assert_msg(furlong_record, "public extension write failed: %s", result_message(furlong_record));
  ck_assert_msg(base_snapshot_unchanged(),
                "first extension write changed the active catalog or a referenced base record");
  auto latest_unit_definition = registry.get_latest_definition_by_type(TypeID{0x43414C5000000002ULL});
  ck_assert_msg(latest_unit_definition, "latest Unit definition lookup failed");
  ck_assert_msg(furlong_record.value->definition_id == latest_unit_definition.value->ref.id,
                "extension writer did not use Unit v2 definition");
  auto with_furlong = load_caliper_catalog(registry, store);
  ck_assert_msg(with_furlong, "extension reload failed: %s", result_message(with_furlong));
  auto furlong_to_meters = convert_caliper_value(*with_furlong.value, "1", "fur", "m");
  ck_assert_msg(furlong_to_meters, "extension conversion failed: %s", result_message(furlong_to_meters));
  ck_assert(std::abs(furlong_to_meters.value->value - 201.168) < 1e-12);
  const auto meter_before_override = std::find_if(
      with_furlong.value->begin(), with_furlong.value->end(),
      [](const CaliperCatalogUnit& unit) { return unit.symbol == "m" && !unit.hidden; });
  ck_assert_msg(meter_before_override != with_furlong.value->end() && meter_before_override->object_id,
                "effective meter object reference is missing");
  ck_assert_msg(*meter_before_override->object_id == original_meter_id,
                "extension write changed the effective base meter identity");

  auto implicit_collision = write_caliper_extension_unit(registry, store, CaliperCatalogUnit{
      "bad meter", "m", "Length", { "user" }, std::nullopt, 1.0, 0.0, false});
  ck_assert(!implicit_collision);
  auto missing_base = write_caliper_extension_unit(registry, store, CaliperCatalogUnit{
      "bad chain", "bad-chain", "Length", { "user" }, std::string("not-a-unit"), 1.0, 0.0, false});
  ck_assert(!missing_base);
  ck_assert_str_eq(missing_base.error->message.c_str(),
      "Caliper extension base references must use a direct object ID");
  auto missing_base_id_unit = CaliperCatalogUnit{
      "missing ID chain", "missing-id", "Length", { "user" }, std::nullopt, 1.0, 0.0, false};
  missing_base_id_unit.base_object_id = ObjectID::random();
  auto missing_base_id = write_caliper_extension_unit(registry, store, missing_base_id_unit);
  ck_assert(!missing_base_id);
  auto second_furlong = CaliperCatalogUnit{
      "second furlong", "fur", "Length", { "user" }, std::string("m"), 2.0, 0.0, false};
  second_furlong.base_object_id = canonical_meter_id;
  auto duplicate_symbol = write_caliper_extension_unit(registry, store, second_furlong);
  ck_assert(!duplicate_symbol);
  auto duplicate_name_unit = CaliperCatalogUnit{
      "furlong", "fur2", "Length", { "user" }, std::string("m"), 2.0, 0.0, false};
  duplicate_name_unit.base_object_id = canonical_meter_id;
  auto duplicate_name = write_caliper_extension_unit(registry, store, duplicate_name_unit);
  ck_assert(!duplicate_name);
  auto ambiguous_name_unit = CaliperCatalogUnit{
      "meter", "m2", "Length", { "user" }, std::string("m"), 2.0, 0.0, false};
  ambiguous_name_unit.base_object_id = canonical_meter_id;
  auto ambiguous_name = write_caliper_extension_unit(registry, store, ambiguous_name_unit);
  ck_assert(!ambiguous_name);
  auto unknown_override = write_caliper_extension_unit(registry, store, CaliperCatalogUnit{
      "unknown override", "not-base", "Length", { "user" }, std::nullopt, 1.0, 0.0, true});
  ck_assert(!unknown_override);
  auto dimension_override = write_caliper_extension_unit(registry, store, CaliperCatalogUnit{
      "wrong meter", "m", "Time", { "user" }, std::nullopt, 1.0, 0.0, true});
  ck_assert(!dimension_override);
  auto caller_identity = furlong;
  caller_identity.symbol = "forged";
  caller_identity.object_id = original_meter_id;
  auto rejected_identity = write_caliper_extension_unit(registry, store, caller_identity);
  ck_assert(!rejected_identity);

  CaliperCatalogUnit meter_override{
    "custom meter", "m", "Length", { "user" }, std::nullopt, 1.0, 0.0, true
  };
  auto override_record = write_caliper_extension_unit(registry, store, meter_override);
  ck_assert_msg(override_record, "explicit same-dimension override failed: %s",
                result_message(override_record));
  ck_assert_msg(base_snapshot_unchanged(),
                "override write changed the active catalog or a referenced base record");
  auto repeated_override = meter_override;
  repeated_override.name = "another custom meter";
  auto rejected_repeated_override = write_caliper_extension_unit(registry, store, repeated_override);
  ck_assert(!rejected_repeated_override);
  auto after_override = load_caliper_catalog(registry, store);
  ck_assert_msg(after_override, "override reload failed: %s", result_message(after_override));
  auto visible_meter = std::find_if(after_override.value->begin(), after_override.value->end(),
      [](const CaliperCatalogUnit& unit) { return unit.symbol == "m" && !unit.hidden; });
  auto hidden_meter = std::find_if(after_override.value->begin(), after_override.value->end(),
      [](const CaliperCatalogUnit& unit) { return unit.symbol == "m" && unit.hidden; });
  ck_assert(visible_meter != after_override.value->end());
  ck_assert(hidden_meter != after_override.value->end());
  ck_assert_str_eq(visible_meter->name.c_str(), "custom meter");
  ck_assert_msg(hidden_meter->object_id == original_meter_id,
                "overridden base identity was not retained for direct references");
  auto immutable_meter_after = store.get_latest(original_meter_id);
  auto immutable_dimension_after = store.get_latest(length_dimension_id);
  auto catalog_after = store.get_latest(active_catalog->ref.id);
  ck_assert_msg(immutable_meter_after && immutable_dimension_after && catalog_after,
                "immutable base objects disappeared");
  ck_assert_msg(immutable_meter_after.value->payload_cbor == original_meter_record.value->payload_cbor,
                "override changed the referenced base Unit payload");
  ck_assert_msg(immutable_dimension_after.value->payload_cbor == original_dimension_record.value->payload_cbor,
                "override changed the referenced base Dimension payload");
  ck_assert_msg(catalog_after.value->payload_cbor == original_catalog_payload,
                "extension writes changed the immutable catalog payload");
  for (const auto& snapshot : immutable_base_records) {
    auto unchanged = store.get_latest(snapshot.ref.id);
    ck_assert_msg(unchanged, "a referenced immutable base object disappeared");
    ck_assert_msg(unchanged.value->ref.ver.v == snapshot.ref.ver.v
                  && unchanged.value->definition_id == snapshot.definition_id
                  && unchanged.value->payload_cbor == snapshot.payload_cbor,
                  "an immutable catalog Unit or Dimension changed after extension writes");
  }
  auto old_direct_chain = convert_caliper_value(*after_override.value, "1", "fur", "m");
  ck_assert_msg(old_direct_chain, "direct reference followed a symbol override: %s",
                result_message(old_direct_chain));
  ck_assert(std::abs(old_direct_chain.value->value - 201.168) < 1e-12);

  CaliperCatalogUnit direct_child{
    "double custom meter", "dcm", "Length", { "user" }, std::string("m"), 2.0, 0.0, false
  };
  direct_child.base_object_id = override_record.value->ref.id;
  auto direct_child_record = write_caliper_extension_unit(registry, store, direct_child);
  ck_assert_msg(direct_child_record, "direct object-ID base reference write failed: %s",
                result_message(direct_child_record));
  auto with_direct_child = load_caliper_catalog(registry, store);
  ck_assert_msg(with_direct_child, "direct reference catalog reload failed: %s",
                result_message(with_direct_child));
  auto direct_conversion = convert_caliper_value(*with_direct_child.value, "1", "dcm", "m");
  ck_assert_msg(direct_conversion, "direct reference conversion failed: %s",
                result_message(direct_conversion));
  ck_assert(std::abs(direct_conversion.value->value - 2.0) < 1e-15);

  auto inch = convert_caliper_value(*catalog.value, "1", "in", "m");
  ck_assert_msg(inch, "inch conversion failed: %s", result_message(inch));
  ck_assert(std::abs(inch.value->value - 0.0254) < 1e-15);

  auto chain = convert_caliper_value(*catalog.value, "1.5", "ft", "cm");
  ck_assert_msg(chain, "chained conversion failed: %s", result_message(chain));
  ck_assert(std::abs(chain.value->value - 45.72) < 1e-12);

  const std::vector<CaliperCatalogUnit> decimal_chain = {
    { "meter", "m", "Length", { "si" }, std::nullopt, std::nullopt, std::nullopt, false },
    { "decimeter", "dm", "Length", { "si" }, std::string("m"), 0.1, 0.0, false },
    { "centimeter", "cm", "Length", { "si" }, std::string("dm"), 0.1, 0.0, false },
  };
  auto exact_chain = convert_caliper_value(decimal_chain, "3", "cm", "m");
  ck_assert_msg(exact_chain, "exact rational chain failed: %s", result_message(exact_chain));
  ck_assert(std::abs(exact_chain.value->value - 0.03) < 1e-15);

  auto freeze = convert_caliper_value(*catalog.value, "32", "°F", "°C");
  ck_assert_msg(freeze, "offset conversion failed: %s", result_message(freeze));
  ck_assert(std::abs(freeze.value->value) < 1e-12);
  ck_assert_str_eq(freeze.value->dimension.c_str(), "Temperature");

  auto speed = convert_caliper_value(*catalog.value, "60", "mph", "km/h");
  ck_assert_msg(speed, "velocity conversion failed: %s", result_message(speed));
  ck_assert(std::abs(speed.value->value - 96.56064) < 1e-10);

  auto unknown = convert_caliper_value(*catalog.value, "1", "unknown", "m");
  ck_assert(!unknown);
  ck_assert_str_eq(unknown.error->message.c_str(), "unknown Caliper unit: unknown");
  auto unknown_target = convert_caliper_value(*catalog.value, "1", "m", "unknown");
  ck_assert(!unknown_target);
  ck_assert_str_eq(unknown_target.error->message.c_str(), "unknown Caliper unit: unknown");
  auto incompatible = convert_caliper_value(*catalog.value, "1", "m", "kg");
  ck_assert(!incompatible);
  ck_assert_str_eq(incompatible.error->message.c_str(), "incompatible Caliper dimensions: Length and Mass");

  const std::vector<std::pair<std::string, std::string>> malformed_values = {
    { "", "invalid decimal value for Caliper conversion" },
    { ".", "invalid decimal value for Caliper conversion" },
    { "1.2.3", "invalid decimal value for Caliper conversion" },
    { "1e", "invalid decimal value for Caliper conversion" },
    { "1e+", "invalid decimal exponent for Caliper conversion" },
    { "1e999", "exact rational Caliper conversion overflow" },
    { "1e-999", "exact rational Caliper conversion overflow" },
    { "--1", "invalid decimal value for Caliper conversion" },
  };
  for (const auto& [malformed, expected_error] : malformed_values) {
    auto invalid = convert_caliper_value(*catalog.value, malformed, "m", "cm");
    ck_assert_msg(!invalid, "malformed decimal accepted: %s", malformed.c_str());
    ck_assert_str_eq(invalid.error->message.c_str(), expected_error.c_str());
  }
  auto reduced_decimal = convert_caliper_value(*catalog.value, "0.5000000000000000000", "m", "m");
  ck_assert_msg(reduced_decimal, "reducible long decimal was rejected: %s", result_message(reduced_decimal));
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(reduced_decimal.value->value),
                    std::bit_cast<std::uint64_t>(0.5));
  auto negative_decimal = convert_caliper_value(*catalog.value, "-0.5000000000000000000", "m", "m");
  ck_assert_msg(negative_decimal, "negative reducible decimal was rejected: %s",
                result_message(negative_decimal));
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(negative_decimal.value->value),
                    std::bit_cast<std::uint64_t>(-0.5));
  auto absolute_zero_celsius = convert_caliper_value(*catalog.value, "-273.15", "°C", "K");
  ck_assert_msg(absolute_zero_celsius, "Celsius absolute-zero boundary conversion failed: %s",
                result_message(absolute_zero_celsius));
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(absolute_zero_celsius.value->value),
                    std::bit_cast<std::uint64_t>(0.0));
  const std::vector<std::pair<std::string, std::string>> absolute_zero_cases = {
      {"K", "-0.01"}, {"K", "0"}, {"K", "0.01"},
      {"°C", "-273.16"}, {"°C", "-273.15"}, {"°C", "-273.14"},
      {"°F", "-459.68"}, {"°F", "-459.67"}, {"°F", "-459.66"},
  };
  for (const auto& [source_symbol, input_value] : absolute_zero_cases) {
    const bool below_zero = input_value == "-0.01" || input_value == "-273.16"
        || input_value == "-459.68";
    for (const auto& target_symbol : {"K", "°C", "°F"}) {
      auto result = convert_caliper_value(*catalog.value, input_value, source_symbol, target_symbol);
      if (below_zero) {
        ck_assert_msg(!result, "below-zero value %s %s converted to %s", input_value.c_str(),
                      source_symbol.c_str(), target_symbol);
        ck_assert_str_eq(result.error->message.c_str(), "Caliper value is below the dimension minimum");
      } else {
        ck_assert_msg(result, "boundary/above-zero value %s %s failed for %s: %s",
                      input_value.c_str(), source_symbol.c_str(), target_symbol, result_message(result));
      }
    }
  }
  for (const auto& source_symbol : {"K", "°C", "°F"}) {
    const std::string boundary = source_symbol == std::string("K") ? "0"
        : source_symbol == std::string("°C") ? "-273.15" : "-459.67";
    auto result = convert_caliper_value(*catalog.value, boundary, source_symbol, "K");
    ck_assert_msg(result, "absolute-zero boundary %s %s failed: %s", boundary.c_str(),
                  source_symbol, result_message(result));
    ck_assert_uint_eq(std::bit_cast<std::uint64_t>(result.value->value),
                      std::bit_cast<std::uint64_t>(0.0));
  }
  auto minimum_integer = convert_caliper_value(*catalog.value, "-9223372036854775808", "m", "m");
  ck_assert_msg(minimum_integer, "INT64_MIN decimal was rejected: %s", result_message(minimum_integer));
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(minimum_integer.value->value),
                    std::bit_cast<std::uint64_t>(-9223372036854775808.0));
  for (const auto outside_domain : {"9223372036854775808", "-9223372036854775809"}) {
    auto invalid = convert_caliper_value(*catalog.value, outside_domain, "m", "m");
    ck_assert_msg(!invalid, "out-of-domain integer accepted: %s", outside_domain);
    ck_assert_str_eq(invalid.error->message.c_str(), "exact rational Caliper conversion overflow");
  }

  const std::vector<CaliperCatalogUnit> rounding_catalog = {
    { "meter", "m", "Length", { "si" }, std::nullopt, std::nullopt, std::nullopt, false },
    { "large meter", "large-m", "Length", { "test" }, std::string("m"), 9007199254740992.0, 0.0, false },
    { "larger meter", "larger-m", "Length", { "test" }, std::string("large-m"), 4.0, 0.0, false },
  };
  auto below_tie = convert_caliper_value(rounding_catalog, "36028797018963971", "m", "larger-m");
  auto at_tie = convert_caliper_value(rounding_catalog, "36028797018963972", "m", "larger-m");
  auto above_tie = convert_caliper_value(rounding_catalog, "36028797018963973", "m", "larger-m");
  ck_assert(below_tie && at_tie && above_tie);
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(below_tie.value->value), 0x3ff0000000000000ULL);
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(at_tie.value->value), 0x3ff0000000000000ULL);
  ck_assert_uint_eq(std::bit_cast<std::uint64_t>(above_tie.value->value), 0x3ff0000000000001ULL);

  auto missing_link_catalog = std::vector<CaliperCatalogUnit>{
    { "missing", "missing", "Length", { "test" }, std::string("absent"), 2.0, 0.0, false },
  };
  auto missing_link = convert_caliper_value(missing_link_catalog, "1", "missing", "missing");
  ck_assert(!missing_link);
  ck_assert_str_eq(missing_link.error->message.c_str(), "unknown base unit in Caliper conversion chain: absent");
  const std::vector<CaliperCatalogUnit> cyclic_catalog = {
    { "cycle a", "cycle-a", "Length", { "test" }, std::string("cycle-b"), 2.0, 0.0, false },
    { "cycle b", "cycle-b", "Length", { "test" }, std::string("cycle-a"), 3.0, 0.0, false },
  };
  auto cycle = convert_caliper_value(cyclic_catalog, "1", "cycle-a", "cycle-a");
  ck_assert(!cycle);
  ck_assert_str_eq(cycle.error->message.c_str(), "cycle in Caliper unit conversion chain");
  const std::vector<CaliperCatalogUnit> dimension_mismatch_catalog = {
    { "meter", "m", "Length", { "si" }, std::nullopt, std::nullopt, std::nullopt, false },
    { "bad meter", "bad-m", "Mass", { "test" }, std::string("m"), 1.0, 0.0, false },
  };
  auto dimension_mismatch = convert_caliper_value(dimension_mismatch_catalog, "1", "bad-m", "bad-m");
  ck_assert(!dimension_mismatch);
  ck_assert_str_eq(dimension_mismatch.error->message.c_str(), "dimension mismatch in Caliper conversion chain");
  for (const auto invalid_scale : {0.0, -1.0, std::numeric_limits<double>::infinity()}) {
    auto invalid_scale_catalog = rounding_catalog;
    invalid_scale_catalog[1].scale = invalid_scale;
    auto invalid = convert_caliper_value(invalid_scale_catalog, "1", "m", "large-m");
    ck_assert(!invalid);
    const auto* expected_error = std::isinf(invalid_scale)
                                   ? "non-finite Caliper conversion factor"
                                   : "Caliper conversion scale must be positive";
    ck_assert_str_eq(invalid.error->message.c_str(), expected_error);
  }
  auto invalid_offset_catalog = rounding_catalog;
  invalid_offset_catalog[1].offset = std::numeric_limits<double>::quiet_NaN();
  auto invalid_offset = convert_caliper_value(invalid_offset_catalog, "1", "m", "large-m");
  ck_assert(!invalid_offset);
  ck_assert_str_eq(invalid_offset.error->message.c_str(), "non-finite Caliper conversion factor");
  const std::vector<CaliperCatalogUnit> invalid_root_scale_catalog = {
    { "bad root", "bad-root", "Length", { "test" }, std::nullopt, 0.0, 0.0, false },
  };
  auto invalid_root_scale = convert_caliper_value(invalid_root_scale_catalog, "1", "bad-root", "bad-root");
  ck_assert(!invalid_root_scale);
  ck_assert_str_eq(invalid_root_scale.error->message.c_str(), "Caliper conversion scale must be positive");
  const std::vector<CaliperCatalogUnit> invalid_root_offset_catalog = {
    { "bad root", "bad-root", "Length", { "test" }, std::nullopt, 1.0,
      std::numeric_limits<double>::infinity(), false },
  };
  auto invalid_root_offset = convert_caliper_value(invalid_root_offset_catalog, "1", "bad-root", "bad-root");
  ck_assert(!invalid_root_offset);
  ck_assert_str_eq(invalid_root_offset.error->message.c_str(), "non-finite Caliper conversion factor");

  auto reloaded_catalog = load_caliper_catalog(registry, store);
  ck_assert_msg(reloaded_catalog, "reloaded catalog lookup failed: %s", result_message(reloaded_catalog));
  auto reloaded_freeze = convert_caliper_value(*reloaded_catalog.value, "32", "°F", "°C");
  ck_assert_msg(reloaded_freeze, "reloaded offset conversion failed: %s", result_message(reloaded_freeze));
  ck_assert_msg(reloaded_freeze.value->value == freeze.value->value,
                "Caliper conversion changed after catalog reload");

  const std::vector<CaliperCatalogUnit> bounded_chain = {
    { "meter", "m", "Length", { "si" }, std::nullopt, std::nullopt, std::nullopt, false },
    { "large_a", "a", "Length", { "test" }, std::string("m"), 1e18, 0.0, false },
    { "large_b", "b", "Length", { "test" }, std::string("a"), 1e18, 0.0, false },
  };
  auto overflow = convert_caliper_value(bounded_chain, "1", "b", "m");
  ck_assert(!overflow);
  ck_assert_str_eq(overflow.error->message.c_str(), "exact rational Caliper conversion overflow");

  ck_assert_msg(store.close(), "close before extension reopen failed");
  {
    SqliteStore reopened_store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(reopened_store.open(), "extension store reopen failed");
    ck_assert_msg(reopened_store.ensure_schema(), "reopened extension schema failed");
    SchemaRegistry reopened_registry(reopened_store);
    auto reopened_catalog = load_caliper_catalog(reopened_registry, reopened_store);
    ck_assert_msg(reopened_catalog, "extension catalog failed after reopen: %s",
                  result_message(reopened_catalog));
    auto reopened_conversion = convert_caliper_value(*reopened_catalog.value, "1", "fur", "m");
    ck_assert_msg(reopened_conversion, "persisted extension conversion failed: %s",
                  result_message(reopened_conversion));
    ck_assert(std::abs(reopened_conversion.value->value - 201.168) < 1e-12);
    ck_assert_msg(reopened_store.close(), "reopened extension store close failed");
  }
  cleanup_temp_db(path);
}
END_TEST

START_TEST(test_caliper_catalog_rejects_wrong_unit_reference)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");
  SchemaRegistry registry(store);
  auto schema = bootstrap_core_schema(registry);
  ck_assert_msg(schema, "schema bootstrap failed: %s", result_message(schema));
  const TypeID dimension_type{0x43414C5000000001ULL};
  const TypeID unit_type{0x43414C5000000002ULL};
  const TypeID catalog_type{0x43414C5000000009ULL};
  auto dimension_definition = registry.get_definition_by_type(dimension_type);
  auto unit_definition = registry.get_latest_definition_by_type(unit_type);
  auto catalog_definition = registry.get_definition_by_type(catalog_type);
  ck_assert(dimension_definition && unit_definition && catalog_definition);
  auto dimension = store.create_object(dimension_type, dimension_definition.value->ref.id,
      nlohmann::json::to_cbor(nlohmann::json{
          { "name", "Length" }, { "symbol", "L" }, { "components", {{"Length", 1}} }
      }));
  ck_assert_msg(dimension, "create fixture dimension failed: %s", result_message(dimension));

  const std::map<std::string, std::pair<std::string, int>> prefixes = {
    { "Q", { "quetta", 30 } }, { "R", { "ronna", 27 } }, { "Y", { "yotta", 24 } },
    { "Z", { "zetta", 21 } }, { "E", { "exa", 18 } }, { "P", { "peta", 15 } },
    { "T", { "tera", 12 } }, { "G", { "giga", 9 } }, { "M", { "mega", 6 } },
    { "k", { "kilo", 3 } }, { "h", { "hecto", 2 } }, { "da", { "deca", 1 } },
    { "d", { "deci", -1 } }, { "c", { "centi", -2 } }, { "m", { "milli", -3 } },
    { "µ", { "micro", -6 } }, { "n", { "nano", -9 } }, { "p", { "pico", -12 } },
    { "f", { "femto", -15 } }, { "a", { "atto", -18 } }, { "z", { "zepto", -21 } },
    { "y", { "yocto", -24 } }, { "r", { "ronto", -27 } }, { "q", { "quecto", -30 } },
  };
  nlohmann::json prefix_payload = nlohmann::json::array();
  for (const auto& [symbol, metadata] : prefixes) {
    prefix_payload.push_back({ { "symbol", symbol }, { "name", metadata.first },
                               { "exponent", metadata.second } });
  }
  const auto wrong_unit_id = ObjectID::random();
  auto wrong_kind = store.create_object_with_id(wrong_unit_id, dimension_type,
      dimension_definition.value->ref.id,
      nlohmann::json::to_cbor(nlohmann::json{
          { "name", "Wrong Kind" }, { "symbol", "X" },
          { "components", {{"Length", 1}} }
      }));
  ck_assert_msg(wrong_kind, "create wrong-kind reference fixture failed: %s",
                result_message(wrong_kind));
  auto catalog_payload = nlohmann::json{
      { "name", "Caliper Base Catalog" }, { "catalog_version", 2 },
      { "dimensions", nlohmann::json::array({
          nlohmann::json{{"name", "Length"}, {"symbol", "L"},
                         {"components", {{"Length", 1}}},
                         {"dimension_id", dimension.value->ref.id.to_hex()}}
      }) },
      { "units", nlohmann::json::array({
          nlohmann::json{{"name", "meter"}, {"symbol", "m"},
                         {"dimension", "Length"}, {"dimension_id", dimension.value->ref.id.to_hex()},
                         {"unit_id", wrong_unit_id.to_hex()}, {"systems", {"SI"}},
                         {"scale", 1.0}, {"offset", 0.0}}
      }) },
      { "prefixes", prefix_payload }
  };
  ObjectID catalog_id{};
  constexpr std::array<std::uint8_t, 16> catalog_id_bytes = {
    'C', 'A', 'L', 'I', 'P', 'E', 'R', '-', 'B', 'A', 'S', 'E', '-', '0', '0', '2'
  };
  catalog_id.bytes = catalog_id_bytes;
  auto malformed_catalog = store.create_object_with_id(catalog_id, catalog_type,
      catalog_definition.value->ref.id, nlohmann::json::to_cbor(catalog_payload));
  ck_assert_msg(malformed_catalog, "create malformed catalog fixture failed: %s",
                result_message(malformed_catalog));
  auto loaded = load_caliper_catalog(registry, store);
  ck_assert(!loaded);
  ck_assert_str_eq(loaded.error->message.c_str(), "invalid Caliper base catalog reference");
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_caliper_catalog_requires_stable_v2_identity_and_version)
{
  constexpr std::array<std::uint8_t, 16> catalog_id_bytes = {
    'C', 'A', 'L', 'I', 'P', 'E', 'R', '-', 'B', 'A', 'S', 'E', '-', '0', '0', '2'
  };
  ObjectID catalog_id{};
  catalog_id.bytes = catalog_id_bytes;

  {
    SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
    ck_assert_msg(store.open(), "open missing-catalog fixture failed");
    ck_assert_msg(store.ensure_schema(), "ensure missing-catalog fixture schema failed");
    SchemaRegistry registry(store);
    auto schema = bootstrap_core_schema(registry);
    ck_assert_msg(schema, "missing-catalog fixture bootstrap failed: %s", result_message(schema));
    auto missing = load_caliper_catalog(registry, store);
    ck_assert(!missing);
    ck_assert_str_eq(missing.error->message.c_str(), "Caliper v2 catalog is missing");
    ck_assert_msg(store.close(), "close missing-catalog fixture failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
    ck_assert_msg(store.open(), "open wrong-type fixture failed");
    ck_assert_msg(store.ensure_schema(), "ensure wrong-type fixture schema failed");
    SchemaRegistry registry(store);
    auto schema = bootstrap_core_schema(registry);
    ck_assert_msg(schema, "wrong-type fixture bootstrap failed: %s", result_message(schema));
    auto types = registry.list_types();
    ck_assert_msg(types, "wrong-type fixture type listing failed");
    auto dimension_type = find_type(types.value.value(), "Caliper", "Dimension");
    ck_assert_msg(dimension_type, "Caliper Dimension type is missing");
    auto dimension_definition = registry.get_latest_definition_by_type(dimension_type->type_id);
    ck_assert_msg(dimension_definition, "Caliper Dimension definition is missing");
    auto wrong_type = store.create_object_with_id(catalog_id, dimension_type->type_id,
        dimension_definition.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{{"name", "wrong catalog type"}}));
    ck_assert_msg(wrong_type, "create wrong-type fixture failed: %s", result_message(wrong_type));
    auto loaded = load_caliper_catalog(registry, store);
    ck_assert(!loaded);
    ck_assert_str_eq(loaded.error->message.c_str(), "Caliper v2 catalog has the wrong object type");
    ck_assert_msg(store.close(), "close wrong-type fixture failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
    ck_assert_msg(store.open(), "open unsupported-version fixture failed");
    ck_assert_msg(store.ensure_schema(), "ensure unsupported-version fixture schema failed");
    SchemaRegistry registry(store);
    auto schema = bootstrap_core_schema(registry);
    ck_assert_msg(schema, "unsupported-version fixture bootstrap failed: %s", result_message(schema));
    const TypeID catalog_type{0x43414C5000000009ULL};
    auto catalog_definition = registry.get_latest_definition_by_type(catalog_type);
    ck_assert_msg(catalog_definition, "Caliper Catalog definition is missing");
    auto unsupported = store.create_object_with_id(catalog_id, catalog_type,
        catalog_definition.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{{"catalog_version", 3}}));
    ck_assert_msg(unsupported, "create unsupported-version fixture failed: %s",
                  result_message(unsupported));
    auto loaded = load_caliper_catalog(registry, store);
    ck_assert(!loaded);
    ck_assert_str_eq(loaded.error->message.c_str(), "unsupported Caliper base catalog version");
    ck_assert_msg(store.close(), "close unsupported-version fixture failed");
  }
}
END_TEST

START_TEST(test_caliper_base_catalog_persists_across_store_reopen)
{
  const auto path = make_temp_db_path();
  referee::Bytes initial_payload;
  referee::Bytes legacy_payload;
  referee::TypeID catalog_type{};
  referee::ObjectID legacy_id{};
  referee::ObjectRecord legacy_catalog_record;
  referee::ObjectRecord legacy_length_dimension;
  referee::ObjectRecord legacy_resistance_dimension;
  referee::ObjectRecord legacy_um;
  referee::ObjectRecord legacy_ohm;
  constexpr std::array<std::uint8_t, 16> legacy_id_bytes = {
    'C', 'A', 'L', 'I', 'P', 'E', 'R', '-', 'B', 'A', 'S', 'E', '-', '0', '0', '1'
  };
  legacy_id.bytes = legacy_id_bytes;

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "open initial store failed");
    ck_assert_msg(store.ensure_schema(), "initial ensure_schema failed");
    SchemaRegistry registry(store);
    auto schema = bootstrap_core_schema(registry);
    ck_assert_msg(schema, "initial schema bootstrap failed: %s", result_message(schema));

    auto types = registry.list_types();
    ck_assert_msg(types, "initial type listing failed: %s", result_message(types));
    auto dimension_type = find_type(types.value.value(), "Caliper", "Dimension");
    auto unit_type = find_type(types.value.value(), "Caliper", "Unit");
    ck_assert_msg(dimension_type.has_value() && unit_type.has_value(), "Caliper schemas missing");
    auto dimension_def = registry.get_definition_by_type(dimension_type->type_id);
    auto unit_def = registry.get_definition_by_type(unit_type->type_id);
    ck_assert_msg(dimension_def && unit_def, "Caliper definitions unavailable");
    auto length_dimension = store.create_object(
        dimension_type->type_id, dimension_def.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{
            { "name", "Length" }, { "symbol", "L" }, { "components", {{"Length", 1}} }
        }));
    auto resistance_dimension = store.create_object(
        dimension_type->type_id, dimension_def.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{
            { "name", "Resistance" }, { "symbol", "M*L2/T3/I2" },
            { "components", {{"Mass", 1}, {"Length", 2}, {"Time", -3}, {"ElectricCurrent", -2}} }
        }));
    ck_assert_msg(length_dimension && resistance_dimension, "legacy dimension fixture creation failed");
    legacy_length_dimension = *length_dimension.value;
    legacy_resistance_dimension = *resistance_dimension.value;
    auto legacy_umR = store.create_object(
        unit_type->type_id, unit_def.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{
            { "name", "micrometer" }, { "symbol", "um" },
            { "dimension_id", length_dimension.value->ref.id.to_hex() },
            { "system", "si" }, { "systems", {"si"} }, { "scale", 0.000001 }, { "offset", 0.0 }
        }));
    ck_assert_msg(legacy_umR, "legacy micrometer record creation failed: %s", result_message(legacy_umR));
    legacy_um = *legacy_umR.value;
    auto legacy_ohmR = store.create_object(
        unit_type->type_id, unit_def.value->ref.id,
        nlohmann::json::to_cbor(nlohmann::json{
            { "name", "ohm" }, { "symbol", "ohm" },
            { "dimension_id", resistance_dimension.value->ref.id.to_hex() },
            { "system", "si" }, { "systems", {"si"} }, { "scale", 1.0 }, { "offset", 0.0 }
        }));
    ck_assert_msg(legacy_ohmR, "legacy ohm record creation failed: %s", result_message(legacy_ohmR));
    legacy_ohm = *legacy_ohmR.value;
    auto catalog_type_summary = find_type(types.value.value(), "Caliper", "Catalog");
    ck_assert_msg(catalog_type_summary.has_value(), "Caliper::Catalog type missing");
    catalog_type = catalog_type_summary->type_id;
    auto catalog_def = registry.get_definition_by_type(catalog_type);
    ck_assert_msg(catalog_def, "Caliper::Catalog definition lookup failed: %s",
                  result_message(catalog_def));
    legacy_payload = nlohmann::json::to_cbor(nlohmann::json{
      { "name", "Caliper Base Catalog" }, { "catalog_version", 1 },
      { "dimensions", nlohmann::json::array({
          nlohmann::json{{"name", "Length"}, {"symbol", "L"}, {"components", {{"Length", 1}}}},
          nlohmann::json{{"name", "Resistance"}, {"symbol", "M*L2/T3/I2"},
                         {"components", {{"Mass", 1}, {"Length", 2}, {"Time", -3}, {"ElectricCurrent", -2}}}}
      }) },
      { "units", nlohmann::json::array({
          nlohmann::json{{"name", "micrometer"}, {"symbol", "um"}, {"dimension", "Length"},
                         {"system", "si"}, {"systems", {"si"}}, {"base_symbol", "m"},
                         {"scale", 0.000001}, {"offset", 0.0}},
          nlohmann::json{{"name", "ohm"}, {"symbol", "ohm"}, {"dimension", "Resistance"},
                         {"system", "si"}, {"systems", {"si"}}, {"scale", 1.0}, {"offset", 0.0}}
      }) }
    });
    auto legacy = store.create_object_with_id(legacy_id, catalog_type, catalog_def.value->ref.id,
                                               legacy_payload);
    ck_assert_msg(legacy, "legacy v1 catalog fixture creation failed: %s", result_message(legacy));
    legacy_catalog_record = *legacy.value;
    auto catalog = bootstrap_core_catalog(registry, store);
    ck_assert_msg(catalog, "initial catalog bootstrap failed: %s", result_message(catalog));
    auto records = store.list_by_type(catalog_type);
    ck_assert_msg(records, "initial catalog lookup failed: %s", result_message(records));
    ck_assert_int_eq((int)records.value->size(), 2);
    bool found_v2 = false;
    for (const auto& record : records.value.value()) {
      if (record.ref.id == legacy_id) {
        ck_assert_msg(record.payload_cbor == legacy_payload, "legacy v1 payload changed during bootstrap");
      } else {
        initial_payload = record.payload_cbor;
        const auto catalog_json = nlohmann::json::from_cbor(record.payload_cbor);
        ck_assert_int_eq(catalog_json.value("catalog_version", 0), 2);
        found_v2 = true;
      }
    }
    ck_assert_msg(found_v2, "catalog v2 was not created");
    auto loaded = load_caliper_catalog(registry, store);
    ck_assert_msg(!loaded, "ambiguous legacy name collision was silently accepted as an extension");
    ck_assert_msg(loaded.error->message == "ambiguous Caliper unit name in extension: micrometer",
                  "legacy collision did not report the stable ambiguity error: %s",
                  loaded.error->message.c_str());
    ck_assert_msg(store.close(), "close initial store failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "reopen store failed");
    ck_assert_msg(store.ensure_schema(), "reopened ensure_schema failed");
    SchemaRegistry registry(store);
    auto schema = bootstrap_core_schema(registry);
    ck_assert_msg(schema, "reopened schema bootstrap failed: %s", result_message(schema));
    auto catalog = bootstrap_core_catalog(registry, store);
    ck_assert_msg(catalog, "reloaded catalog failed: %s", result_message(catalog));

    auto records = store.list_by_type(catalog_type);
    ck_assert_msg(records, "reloaded catalog lookup failed: %s", result_message(records));
    ck_assert_int_eq((int)records.value->size(), 2);
    bool found_v2 = false;
    bool found_v1 = false;
    auto reopened_um = store.get_latest(legacy_um.ref.id);
    auto reopened_ohm = store.get_latest(legacy_ohm.ref.id);
    auto reopened_length = store.get_latest(legacy_length_dimension.ref.id);
    auto reopened_resistance = store.get_latest(legacy_resistance_dimension.ref.id);
    auto reopened_legacy_catalog = store.get_latest(legacy_id);
    ck_assert_msg(reopened_um && reopened_ohm, "legacy unit records were not retained");
    ck_assert_msg(reopened_length && reopened_resistance && reopened_legacy_catalog,
                  "legacy catalog graph records were not retained");
    ck_assert_msg(reopened_um.value->payload_cbor == legacy_um.payload_cbor
                      && reopened_um.value->definition_id == legacy_um.definition_id
                      && reopened_um.value->ref == legacy_um.ref,
                  "legacy micrometer object changed after v2 bootstrap/reopen");
    ck_assert_msg(reopened_ohm.value->payload_cbor == legacy_ohm.payload_cbor
                      && reopened_ohm.value->definition_id == legacy_ohm.definition_id
                      && reopened_ohm.value->ref == legacy_ohm.ref,
                  "legacy ohm object changed after v2 bootstrap/reopen");
    ck_assert_msg(reopened_length.value->payload_cbor == legacy_length_dimension.payload_cbor
                      && reopened_length.value->definition_id == legacy_length_dimension.definition_id
                      && reopened_length.value->ref == legacy_length_dimension.ref,
                  "legacy Length dimension changed after v2 bootstrap/reopen");
    ck_assert_msg(reopened_resistance.value->payload_cbor == legacy_resistance_dimension.payload_cbor
                      && reopened_resistance.value->definition_id == legacy_resistance_dimension.definition_id
                      && reopened_resistance.value->ref == legacy_resistance_dimension.ref,
                  "legacy Resistance dimension changed after v2 bootstrap/reopen");
    ck_assert_msg(reopened_legacy_catalog.value->payload_cbor == legacy_catalog_record.payload_cbor
                      && reopened_legacy_catalog.value->definition_id == legacy_catalog_record.definition_id
                      && reopened_legacy_catalog.value->ref == legacy_catalog_record.ref,
                  "legacy catalog object changed after v2 bootstrap/reopen");
    for (const auto& record : records.value.value()) {
      if (record.ref.id == legacy_id) {
        ck_assert_msg(record.payload_cbor == legacy_payload,
                      "legacy Caliper v1 catalog changed after store reopen");
        found_v1 = true;
      } else {
        ck_assert_msg(record.payload_cbor == initial_payload,
                      "persisted Caliper v2 catalog changed after store reopen");
        found_v2 = true;
      }
    }
    ck_assert_msg(found_v1 && found_v2, "both legacy v1 and active v2 catalogs must persist");
    auto conversion_catalog = load_caliper_catalog(registry, store);
    ck_assert_msg(!conversion_catalog,
                  "ambiguous unreferenced legacy unit names were silently accepted after reopen");
    ck_assert_msg(conversion_catalog.error->message
                      == "ambiguous Caliper unit name in extension: micrometer",
                  "legacy name collision changed after reopen: %s",
                  conversion_catalog.error->message.c_str());
    ck_assert_msg(store.close(), "close reopened store failed");
  }

  cleanup_temp_db(path);
}
END_TEST

START_TEST(test_bootstrap_kernel_io_ops)
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

  auto io_type = find_type(types, "Kernel", "Io");
  auto channel_type = find_type(types, "Kernel", "IoChannel");
  auto datagram_type = find_type(types, "Kernel", "IoDatagram");

  ck_assert_msg(io_type.has_value(), "Kernel::Io missing");
  ck_assert_msg(channel_type.has_value(), "Kernel::IoChannel missing");
  ck_assert_msg(datagram_type.has_value(), "Kernel::IoDatagram missing");

  auto io_def = registry.get_definition_by_type(io_type->type_id);
  ck_assert_msg(io_def, "Kernel::Io definition lookup failed: %s", result_message(io_def));
  ck_assert_msg(type_has_operation(io_def.value.value(), "open_channel"), "Kernel::Io missing open_channel");
  ck_assert_msg(type_has_operation(io_def.value.value(), "open_datagram"), "Kernel::Io missing open_datagram");

  auto channel_def = registry.get_definition_by_type(channel_type->type_id);
  ck_assert_msg(channel_def, "Kernel::IoChannel definition lookup failed: %s", result_message(channel_def));
  ck_assert_msg(type_has_operation(channel_def.value.value(), "send"), "Kernel::IoChannel missing send");
  ck_assert_msg(type_has_operation(channel_def.value.value(), "recv"), "Kernel::IoChannel missing recv");
  ck_assert_msg(type_has_operation(channel_def.value.value(), "await_readable"),
                "Kernel::IoChannel missing await_readable");
  ck_assert_msg(type_has_operation(channel_def.value.value(), "close"), "Kernel::IoChannel missing close");

  auto datagram_def = registry.get_definition_by_type(datagram_type->type_id);
  ck_assert_msg(datagram_def, "Kernel::IoDatagram definition lookup failed: %s", result_message(datagram_def));
  ck_assert_msg(type_has_operation(datagram_def.value.value(), "send"), "Kernel::IoDatagram missing send");
  ck_assert_msg(type_has_operation(datagram_def.value.value(), "recv"), "Kernel::IoDatagram missing recv");
  ck_assert_msg(type_has_operation(datagram_def.value.value(), "await_readable"),
                "Kernel::IoDatagram missing await_readable");
  ck_assert_msg(type_has_operation(datagram_def.value.value(), "close"), "Kernel::IoDatagram missing close");

  const auto* open_channel_op = find_operation(io_def.value.value(), "open_channel");
  ck_assert_msg(open_channel_op != nullptr, "Kernel::Io open_channel not found");
  ck_assert_int_eq((int)open_channel_op->scope, (int)OperationScope::Class);
  ck_assert_int_eq((int)open_channel_op->signature.params.size(), 2);
  ck_assert_int_eq((int)open_channel_op->signature.outputs.size(), 2);
  ck_assert_int_eq((int)open_channel_op->effects.size(), 1);
  ck_assert_int_eq((int)open_channel_op->effects[0].kind, (int)OperationEffectKind::UsesIo);
  ck_assert_str_eq(open_channel_op->effects[0].target.c_str(), "kernel.io.channel");
  ck_assert_msg(open_channel_op->documentation.has_value(), "open_channel documentation missing");

  ck_assert_msg(find_type(types, "Refract", "OperationEffect").has_value(),
                "Refract::OperationEffect missing");
  auto documentation_type = find_type(types, "Refract", "DocumentationMetadata");
  ck_assert_msg(documentation_type.has_value(), "Refract::DocumentationMetadata missing");
  ck_assert_msg(documentation_type->documentation.has_value(),
                "DocumentationMetadata summary missing");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* refract_bootstrap_suite(void) {
  Suite* s = suite_create("RefractBootstrap");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_bootstrap_idempotent);
  tcase_add_test(tc, test_bootstrap_crate_collections);
  tcase_add_test(tc, test_bootstrap_crate_set_index_signature_reopen);
  tcase_add_test(tc, test_bootstrap_core_ops_on_primitives);
  tcase_add_test(tc, test_bootstrap_conch_types);
  tcase_add_test(tc, test_bootstrap_astra_math_types);
  tcase_add_test(tc, test_caliper_quantity_unit_attachment_roundtrip);
  tcase_add_test(tc, test_bootstrap_migrates_astra_float_formats);
  tcase_add_test(tc, test_bootstrap_float_format_migration_rejects_invalid_double_atomically);
  tcase_add_test(tc, test_bootstrap_caliper_units);
  tcase_add_test(tc, test_compose_caliper_catalog_extension_precedence);
  tcase_add_test(tc, test_caliper_runtime_conversion_and_catalog_loading);
  tcase_add_test(tc, test_caliper_catalog_rejects_wrong_unit_reference);
  tcase_add_test(tc, test_caliper_catalog_requires_stable_v2_identity_and_version);
  tcase_add_test(tc, test_caliper_base_catalog_persists_across_store_reopen);
  tcase_add_test(tc, test_bootstrap_kernel_io_ops);

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
