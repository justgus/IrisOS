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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

using namespace referee;
using namespace iris::refract;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

std::optional<TypeSummary> find_type_summary(SchemaRegistry& registry,
                                             const std::string& ns,
                                             const std::string& name) {
  auto listR = registry.list_types();
  if (!listR) return std::nullopt;
  for (const auto& summary : listR.value.value()) {
    if (summary.namespace_name == ns && summary.name == name) {
      return summary;
    }
  }
  return std::nullopt;
}

std::string make_temp_db_path() {
  char tmpl[] = "/tmp/iris_referee_XXXXXX";
  int fd = mkstemp(tmpl);
  if (fd >= 0) close(fd);
  std::string path(tmpl);
  std::remove(path.c_str());
  return path;
}

void cleanup_db_files(const std::string& path) {
  std::remove(path.c_str());
  std::string shm = path + "-shm";
  std::string wal = path + "-wal";
  std::remove(shm.c_str());
  std::remove(wal.c_str());
}

} // namespace

START_TEST(test_phase6_persistence_roundtrip)
{
  std::string db_path = make_temp_db_path();

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    SchemaRegistry registry(store);
    auto boot = iris::refract::bootstrap_core_schema(registry);
    ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

    auto obj1 = store.create_object(TypeID{0x1234ULL}, ObjectID::random(), Bytes{0x01, 0x02});
    ck_assert_msg(obj1, "create obj1 failed: %s", result_message(obj1));
    auto obj2 = store.create_object(TypeID{0x5678ULL}, ObjectID::random(), Bytes{0x03, 0x04});
    ck_assert_msg(obj2, "create obj2 failed: %s", result_message(obj2));

    Bytes props;
    auto edgeR = store.add_edge(obj1.value->ref, obj2.value->ref, "link", "test", props);
    ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

    ck_assert_msg(store.close(), "close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    auto latest1 = store.list_by_type(TypeID{0x1234ULL});
    ck_assert_msg(latest1, "list_by_type failed: %s", result_message(latest1));
    ck_assert_msg(!latest1.value->empty(), "expected obj1 to persist");

    auto latest2 = store.list_by_type(TypeID{0x5678ULL});
    ck_assert_msg(latest2, "list_by_type failed: %s", result_message(latest2));
    ck_assert_msg(!latest2.value->empty(), "expected obj2 to persist");

    auto ref1 = latest1.value->front().ref;
    auto edges = store.edges_from(ref1);
    ck_assert_msg(edges, "edges_from failed: %s", result_message(edges));
    bool found = false;
    for (const auto& edge : edges.value.value()) {
      if (edge.name == "link" && edge.role == "test") {
        found = true;
        break;
      }
    }
    ck_assert_msg(found, "expected edge to persist");

    ck_assert_msg(store.close(), "close failed");
  }

  cleanup_db_files(db_path);
}
END_TEST

START_TEST(test_phase6_demo_persistence)
{
  std::string db_path = make_temp_db_path();
  ObjectID demo_id{};
  ObjectID summary_id{};
  ObjectID detail_id{};

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    SchemaRegistry registry(store);
    auto boot = iris::refract::bootstrap_core_schema(registry);
    ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

    auto demo = find_type_summary(registry, "Demo", "PropulsionSynth");
    ck_assert_msg(demo.has_value(), "expected Demo::PropulsionSynth type");
    auto summary = find_type_summary(registry, "Demo", "Summary");
    ck_assert_msg(summary.has_value(), "expected Demo::Summary type");
    auto detail = find_type_summary(registry, "Demo", "Detail");
    ck_assert_msg(detail.has_value(), "expected Demo::Detail type");

    auto demo_payload = cbor_from_json_string("{\"name\":\"PropulsionSynth\"}");
    auto demoR = store.create_object(demo->type_id, demo->definition_id, demo_payload);
    ck_assert_msg(demoR, "create demo failed: %s", result_message(demoR));
    demo_id = demoR.value->ref.id;

    auto summary_payload = cbor_from_json_string("{\"title\":\"Summary\",\"level\":0}");
    auto summaryR = store.create_object(summary->type_id, summary->definition_id, summary_payload);
    ck_assert_msg(summaryR, "create summary failed: %s", result_message(summaryR));
    summary_id = summaryR.value->ref.id;

    auto detail_payload = cbor_from_json_string("{\"title\":\"Detail\",\"level\":1,\"index\":1}");
    auto detailR = store.create_object(detail->type_id, detail->definition_id, detail_payload);
    ck_assert_msg(detailR, "create detail failed: %s", result_message(detailR));
    detail_id = detailR.value->ref.id;

    Bytes props;
    auto edge1 = store.add_edge(demoR.value->ref, summaryR.value->ref, "summary", "root", props);
    ck_assert_msg(edge1, "add demo->summary failed: %s", result_message(edge1));
    auto edge2 = store.add_edge(summaryR.value->ref, detailR.value->ref, "summarizes", "detail", props);
    ck_assert_msg(edge2, "add summary->detail failed: %s", result_message(edge2));

    ck_assert_msg(store.close(), "close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    auto demoRec = store.get_latest(demo_id);
    ck_assert_msg(demoRec, "get_latest demo failed: %s", result_message(demoRec));
    ck_assert_msg(demoRec.value->has_value(), "expected demo record");

    auto summaryRec = store.get_latest(summary_id);
    ck_assert_msg(summaryRec, "get_latest summary failed: %s", result_message(summaryRec));
    ck_assert_msg(summaryRec.value->has_value(), "expected summary record");

    auto detailRec = store.get_latest(detail_id);
    ck_assert_msg(detailRec, "get_latest detail failed: %s", result_message(detailRec));
    ck_assert_msg(detailRec.value->has_value(), "expected detail record");

    auto demoEdges = store.edges_from(demoRec.value->value().ref);
    ck_assert_msg(demoEdges, "edges_from demo failed: %s", result_message(demoEdges));
    bool found_summary = false;
    for (const auto& edge : demoEdges.value.value()) {
      if (edge.name == "summary" && edge.role == "root"
          && edge.to.id.to_hex() == summary_id.to_hex()) {
        found_summary = true;
        break;
      }
    }
    ck_assert_msg(found_summary, "expected demo->summary edge");

    auto summaryEdges = store.edges_from(summaryRec.value->value().ref);
    ck_assert_msg(summaryEdges, "edges_from summary failed: %s", result_message(summaryEdges));
    bool found_detail = false;
    for (const auto& edge : summaryEdges.value.value()) {
      if (edge.name == "summarizes" && edge.role == "detail"
          && edge.to.id.to_hex() == detail_id.to_hex()) {
        found_detail = true;
        break;
      }
    }
    ck_assert_msg(found_detail, "expected summary->detail edge");

    ck_assert_msg(store.close(), "close failed");
  }

  cleanup_db_files(db_path);
}
END_TEST

START_TEST(test_phase6_definition_migration)
{
  std::string db_path = make_temp_db_path();

  SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry registry(store);

  TypeDefinition v1;
  v1.type_id = TypeID{0xDADAULL};
  v1.name = "Migrated";
  v1.namespace_name = "Test";
  v1.version = 1;

  auto reg1 = registry.register_definition(v1);
  ck_assert_msg(reg1, "register v1 failed: %s", result_message(reg1));

  TypeDefinition v2 = v1;
  v2.version = 2;
  auto reg2 = registry.register_definition(v2);
  ck_assert_msg(reg2, "register v2 failed: %s", result_message(reg2));

  auto latest = registry.get_latest_definition_by_type(v1.type_id);
  ck_assert_msg(latest, "get_latest_definition_by_type failed: %s", result_message(latest));
  ck_assert_msg(latest.value->has_value(), "expected latest definition");
  ck_assert_uint_eq(latest.value->value().definition.version, 2U);

  ck_assert_msg(store.close(), "close failed");
  cleanup_db_files(db_path);
}
END_TEST

Suite* phase6_persistence_suite(void) {
  Suite* s = suite_create("Phase6Persistence");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_phase6_persistence_roundtrip);
  tcase_add_test(tc, test_phase6_definition_migration);
  tcase_add_test(tc, test_phase6_demo_persistence);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = phase6_persistence_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
