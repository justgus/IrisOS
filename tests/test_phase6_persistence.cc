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
