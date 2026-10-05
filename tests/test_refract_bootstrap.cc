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
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <set>
#include <string>
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

  auto array_type = find_type(types, "Crate", "Array");
  ck_assert_msg(array_type.has_value(), "Crate::Array missing");
  ck_assert_msg(find_type(types, "Crate", "List").has_value(), "Crate::List missing");
  ck_assert_msg(find_type(types, "Crate", "Set").has_value(), "Crate::Set missing");
  ck_assert_msg(find_type(types, "Crate", "Map").has_value(), "Crate::Map missing");
  ck_assert_msg(find_type(types, "Crate", "Tuple").has_value(), "Crate::Tuple missing");

  auto defR = registry.get_definition_by_type(array_type->type_id);
  ck_assert_msg(defR, "get_definition_by_type failed: %s", result_message(defR));
  ck_assert_msg(defR.value->has_value(), "Crate::Array definition missing");

  const auto& record = defR.value->value();
  ck_assert_msg(type_has_operation(record, "size"), "Crate::Array missing size op");
  ck_assert_msg(type_has_operation(record, "iterate"), "Crate::Array missing iterate op");
  ck_assert_msg(type_has_operation(record, "index"), "Crate::Array missing index op");
  ck_assert_msg(type_has_operation(record, "contains"), "Crate::Array missing contains op");

  ck_assert_msg(store.close(), "close failed");
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
  ck_assert_msg(string_def.value->has_value(), "String definition missing");
  ck_assert_msg(type_has_operation(string_def.value->value(), "to_string"), "String missing to_string");
  ck_assert_msg(type_has_operation(string_def.value->value(), "print"), "String missing print");
  ck_assert_msg(type_has_operation(string_def.value->value(), "render"), "String missing render");
  ck_assert_msg(type_has_operation(string_def.value->value(), "compare"), "String missing compare");

  auto u64_def = registry.get_definition_by_type(u64_type->type_id);
  ck_assert_msg(u64_def, "U64 definition lookup failed: %s", result_message(u64_def));
  ck_assert_msg(u64_def.value->has_value(), "U64 definition missing");
  ck_assert_msg(type_has_operation(u64_def.value->value(), "to_string"), "U64 missing to_string");
  ck_assert_msg(type_has_operation(u64_def.value->value(), "print"), "U64 missing print");
  ck_assert_msg(type_has_operation(u64_def.value->value(), "render"), "U64 missing render");
  ck_assert_msg(type_has_operation(u64_def.value->value(), "compare"), "U64 missing compare");

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
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
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

  ck_assert_msg(find_type(types, "Astra", "Float").has_value(), "Astra::Float missing");
  ck_assert_msg(find_type(types, "Astra", "Double").has_value(), "Astra::Double missing");
  ck_assert_msg(vector_type.has_value(), "Astra::Vector missing");
  ck_assert_msg(matrix_type.has_value(), "Astra::Matrix missing");
  ck_assert_msg(tensor_type.has_value(), "Astra::Tensor missing");

  auto vector_def = registry.get_definition_by_type(vector_type->type_id);
  ck_assert_msg(vector_def, "Astra::Vector definition lookup failed: %s", result_message(vector_def));
  ck_assert_msg(vector_def.value->has_value(), "Astra::Vector definition missing");
  ck_assert_msg(type_params_match(vector_def.value->value(), {"T", "N"}),
                "Astra::Vector type params missing");

  auto matrix_def = registry.get_definition_by_type(matrix_type->type_id);
  ck_assert_msg(matrix_def, "Astra::Matrix definition lookup failed: %s", result_message(matrix_def));
  ck_assert_msg(matrix_def.value->has_value(), "Astra::Matrix definition missing");
  ck_assert_msg(type_params_match(matrix_def.value->value(), {"T", "R", "C"}),
                "Astra::Matrix type params missing");

  auto tensor_def = registry.get_definition_by_type(tensor_type->type_id);
  ck_assert_msg(tensor_def, "Astra::Tensor definition lookup failed: %s", result_message(tensor_def));
  ck_assert_msg(tensor_def.value->has_value(), "Astra::Tensor definition missing");
  ck_assert_msg(type_params_match(tensor_def.value->value(), {"T", "Dims..."}),
                "Astra::Tensor type params missing");

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
  ck_assert_msg(io_def.value->has_value(), "Kernel::Io definition missing");
  ck_assert_msg(type_has_operation(io_def.value->value(), "open_channel"), "Kernel::Io missing open_channel");
  ck_assert_msg(type_has_operation(io_def.value->value(), "open_datagram"), "Kernel::Io missing open_datagram");

  auto channel_def = registry.get_definition_by_type(channel_type->type_id);
  ck_assert_msg(channel_def, "Kernel::IoChannel definition lookup failed: %s", result_message(channel_def));
  ck_assert_msg(channel_def.value->has_value(), "Kernel::IoChannel definition missing");
  ck_assert_msg(type_has_operation(channel_def.value->value(), "send"), "Kernel::IoChannel missing send");
  ck_assert_msg(type_has_operation(channel_def.value->value(), "recv"), "Kernel::IoChannel missing recv");
  ck_assert_msg(type_has_operation(channel_def.value->value(), "await_readable"),
                "Kernel::IoChannel missing await_readable");
  ck_assert_msg(type_has_operation(channel_def.value->value(), "close"), "Kernel::IoChannel missing close");

  auto datagram_def = registry.get_definition_by_type(datagram_type->type_id);
  ck_assert_msg(datagram_def, "Kernel::IoDatagram definition lookup failed: %s", result_message(datagram_def));
  ck_assert_msg(datagram_def.value->has_value(), "Kernel::IoDatagram definition missing");
  ck_assert_msg(type_has_operation(datagram_def.value->value(), "send"), "Kernel::IoDatagram missing send");
  ck_assert_msg(type_has_operation(datagram_def.value->value(), "recv"), "Kernel::IoDatagram missing recv");
  ck_assert_msg(type_has_operation(datagram_def.value->value(), "await_readable"),
                "Kernel::IoDatagram missing await_readable");
  ck_assert_msg(type_has_operation(datagram_def.value->value(), "close"), "Kernel::IoDatagram missing close");

  const auto* open_channel_op = find_operation(io_def.value->value(), "open_channel");
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
  tcase_add_test(tc, test_bootstrap_core_ops_on_primitives);
  tcase_add_test(tc, test_bootstrap_conch_types);
  tcase_add_test(tc, test_bootstrap_astra_math_types);
  tcase_add_test(tc, test_bootstrap_caliper_units);
  tcase_add_test(tc, test_compose_caliper_catalog_extension_precedence);
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
