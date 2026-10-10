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
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
    ck_assert_msg(store.close(), "old store close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "reopen old store failed");
    ck_assert_msg(store.ensure_schema(), "reopened old store ensure_schema failed");
    SchemaRegistry registry(store);

    auto migrated = bootstrap_core_schema(registry);
    ck_assert_msg(migrated, "format migration failed: %s", result_message(migrated));
    ck_assert_uint_eq(migrated.value->inserted, 2U);

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
    "mm", "um", "nm", "ms", "us", "gal", "fl_oz", "cup", "pt", "qt", "nmi", "kn"
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
  ck_assert_int_eq(saved_catalog.value("catalog_version", 0), 1);
  ck_assert_int_eq((int)saved_catalog.at("units").size(), (int)symbols.size());
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
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
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
  auto incompatible = convert_caliper_value(*catalog.value, "1", "m", "kg");
  ck_assert(!incompatible);

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

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_caliper_base_catalog_persists_across_store_reopen)
{
  const auto path = make_temp_db_path();
  referee::Bytes initial_payload;
  referee::TypeID catalog_type{};

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    ck_assert_msg(store.open(), "open initial store failed");
    ck_assert_msg(store.ensure_schema(), "initial ensure_schema failed");
    SchemaRegistry registry(store);
    auto schema = bootstrap_core_schema(registry);
    ck_assert_msg(schema, "initial schema bootstrap failed: %s", result_message(schema));
    auto catalog = bootstrap_core_catalog(registry, store);
    ck_assert_msg(catalog, "initial catalog bootstrap failed: %s", result_message(catalog));

    auto types = registry.list_types();
    ck_assert_msg(types, "initial type listing failed: %s", result_message(types));
    auto catalog_type_summary = find_type(types.value.value(), "Caliper", "Catalog");
    ck_assert_msg(catalog_type_summary.has_value(), "Caliper::Catalog type missing");
    catalog_type = catalog_type_summary->type_id;
    auto records = store.list_by_type(catalog_type);
    ck_assert_msg(records, "initial catalog lookup failed: %s", result_message(records));
    ck_assert_int_eq((int)records.value->size(), 1);
    initial_payload = records.value->front().payload_cbor;
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
    ck_assert_int_eq((int)records.value->size(), 1);
    ck_assert_msg(records.value->front().payload_cbor == initial_payload,
                  "persisted Caliper base catalog changed after store reopen");
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
