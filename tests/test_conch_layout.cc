extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "conch/session_growth.h"
#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <unistd.h>

#include <nlohmann/json.hpp>

using namespace referee;
using namespace iris::conch;
using namespace iris::refract;

namespace {

template <typename T>
const char* result_message(const Result<T>& result) {
  return result.error.has_value() ? result.error->message.c_str() : "ok";
}

std::string make_temp_path() {
  char path[] = "/tmp/iris-conch-layout-XXXXXX";
  const int fd = ::mkstemp(path);
  ck_assert_msg(fd >= 0, "mkstemp failed");
  ::close(fd);
  return path;
}

void remove_db_files(const std::string& path) {
  std::remove(path.c_str());
  std::remove((path + "-shm").c_str());
  std::remove((path + "-wal").c_str());
}

TypeSummary find_type(SchemaRegistry& registry, const std::string& name) {
  auto typesR = registry.list_types();
  ck_assert_msg(typesR, "list_types failed: %s", result_message(typesR));
  for (const auto& type : typesR.value.value()) {
    if (type.namespace_name == "Conch" && type.name == name) return type;
  }
  ck_abort_msg("Conch::%s type missing", name.c_str());
  return {};
}

ObjectRef create_concho(SchemaRegistry& registry, SqliteStore& store, const std::string& title) {
  const auto type = find_type(registry, "Concho");
  nlohmann::json payload;
  payload["title"] = title;
  auto createR = store.create_object(type.type_id, type.definition_id,
                                    nlohmann::json::to_cbor(payload));
  ck_assert_msg(createR, "create Concho failed: %s", result_message(createR));
  return createR.value->ref;
}

void bootstrap_store(SqliteStore& store, SchemaRegistry& registry) {
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");
  auto bootstrapR = bootstrap_core_schema(registry);
  ck_assert_msg(bootstrapR, "bootstrap failed: %s", result_message(bootstrapR));
}

} // namespace

START_TEST(test_workspace_layout_is_ordered_nested_idempotent_and_persistent)
{
  const auto path = make_temp_path();
  ObjectRef sessionRef{};
  std::string originalTree;
  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    SchemaRegistry registry(store);
    bootstrap_store(store, registry);

    auto cursorR = store.graph_cursor();
    ck_assert_msg(cursorR, "graph_cursor failed: %s", result_message(cursorR));
    auto sessionR = create_session(registry, store, "layout", cursorR.value.value());
    ck_assert_msg(sessionR, "create_session failed: %s", result_message(sessionR));
    auto session = sessionR.value.value();
    sessionRef = session.session;
    auto workspaceEdgesR = store.edges_from(session.session, "workspace", "workspace");
    ck_assert_msg(workspaceEdgesR, "workspace lookup failed: %s",
                  result_message(workspaceEdgesR));
    ck_assert_uint_eq(workspaceEdgesR.value->size(), 1U);

    auto first = create_concho(registry, store, "First");
    auto nested = create_concho(registry, store, "Nested");
    auto second = create_concho(registry, store, "Second");
    ck_assert_msg(store.add_edge(session.session, nested, "contains", "concho", {}),
                  "link nested Concho failed");
    ck_assert_msg(store.add_edge(session.session, first, "contains", "concho", {}),
                  "link first Concho failed");
    ck_assert_msg(store.add_edge(session.session, second, "contains", "concho", {}),
                  "link second Concho failed");

    auto treeR = workspace_tree(registry, store, session.session);
    ck_assert_msg(treeR, "workspace_tree failed: %s", result_message(treeR));
    const auto initialTree = treeR.value.value();
    const auto initialExpected = "Workspace\n  - Nested [" + nested.id.to_hex()
        + "]\n  - First [" + first.id.to_hex() + "]\n  - Second ["
        + second.id.to_hex() + "]\n";
    ck_assert_msg(initialTree == initialExpected,
                  "root tiles do not follow graph-observation order: %s", initialTree.c_str());

    ck_assert_msg(store.add_edge(first, nested, "contains", "concho", {}),
                  "link delayed nested ownership failed");
    auto nestedTreeR = workspace_tree(registry, store, session.session);
    ck_assert_msg(nestedTreeR, "nested workspace_tree failed: %s",
                  result_message(nestedTreeR));
    originalTree = nestedTreeR.value.value();
    const auto nestedExpected = "Workspace\n  - First [" + first.id.to_hex()
        + "]\n    - Nested [" + nested.id.to_hex() + "]\n  - Second ["
        + second.id.to_hex() + "]\n";
    ck_assert_msg(originalTree == nestedExpected,
                  "late ownership did not produce the stable nested tree: %s",
                  originalTree.c_str());

    auto repeatR = workspace_tree(registry, store, session.session);
    ck_assert_msg(repeatR, "repeat workspace_tree failed: %s", result_message(repeatR));
    ck_assert_msg(repeatR.value.value() == originalTree,
                  "repeated update changed tree order or duplicated tiles");
    ck_assert_msg(store.close(), "close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=path, .enable_wal=false });
    SchemaRegistry registry(store);
    bootstrap_store(store, registry);
    auto cursorR = store.graph_cursor();
    ck_assert_msg(cursorR, "reopened graph_cursor failed: %s", result_message(cursorR));
    auto persistedTreeR = workspace_tree(registry, store, sessionRef);
    ck_assert_msg(persistedTreeR, "persisted workspace_tree failed: %s",
                  result_message(persistedTreeR));
    ck_assert_msg(persistedTreeR.value.value() == originalTree,
                  "reopening the store changed the persisted session tree");

    auto newSessionR = create_session(registry, store, "reopened", cursorR.value.value());
    ck_assert_msg(newSessionR, "create reopened session failed: %s", result_message(newSessionR));
    ck_assert_msg(!(newSessionR.value->session == sessionRef),
                  "expected each shell session to retain a distinct identity");

    auto originalWorkspaceR = store.edges_from(sessionRef, "workspace", "workspace");
    auto newWorkspaceR = store.edges_from(newSessionR.value->session, "workspace", "workspace");
    ck_assert_msg(originalWorkspaceR, "original workspace lookup failed: %s",
                  result_message(originalWorkspaceR));
    ck_assert_msg(newWorkspaceR, "new workspace lookup failed: %s",
                  result_message(newWorkspaceR));
    ck_assert_uint_eq(originalWorkspaceR.value->size(), 1U);
    ck_assert_uint_eq(newWorkspaceR.value->size(), 1U);
    ck_assert_msg(!(originalWorkspaceR.value->front().to == newWorkspaceR.value->front().to),
                  "distinct sessions must have distinct workspace roots");

    auto treeR = workspace_tree(registry, store, newSessionR.value->session);
    ck_assert_msg(treeR, "reopened workspace_tree failed: %s", result_message(treeR));
    ck_assert_msg(treeR.value.value() == "Workspace\n",
                  "new session inherited tiles from the previous session: %s",
                  treeR.value->c_str());

    auto newOwner = create_concho(registry, store, "New owner");
    auto laterChild = create_concho(registry, store, "Later child");
    ck_assert_msg(store.add_edge(newSessionR.value->session, newOwner,
                                 "contains", "concho", {}),
                  "link new owner Concho failed");
    ck_assert_msg(store.add_edge(newSessionR.value->session, laterChild,
                                 "contains", "concho", {}),
                  "link later child Concho failed");
    ck_assert_msg(store.add_edge(newOwner, laterChild, "contains", "concho", {}),
                  "link later nested ownership failed");
    auto extendedTreeR = workspace_tree(registry, store, newSessionR.value->session);
    ck_assert_msg(extendedTreeR, "extended workspace_tree failed: %s",
                  result_message(extendedTreeR));
    ck_assert_msg(extendedTreeR.value.value().find("- New owner") != std::string::npos,
                  "new session omitted its own Concho: %s", extendedTreeR.value->c_str());
    ck_assert_msg(extendedTreeR.value.value().find("  - Later child") != std::string::npos,
                  "new session did not nest a child under its own workspace tile");
    ck_assert_msg(extendedTreeR.value.value().find("- First") == std::string::npos,
                  "new session workspace contains an earlier session's Concho");

    auto unchangedOriginalR = workspace_tree(registry, store, sessionRef);
    ck_assert_msg(unchangedOriginalR, "original workspace_tree update failed: %s",
                  result_message(unchangedOriginalR));
    ck_assert_msg(unchangedOriginalR.value.value() == originalTree,
                  "updating a new session changed the previous session's workspace");

    const auto sessionType = find_type(registry, "Session");
    nlohmann::json legacySessionPayload;
    legacySessionPayload["name"] = "legacy-shared-workspace";
    auto legacySessionR = store.create_object(sessionType.type_id, sessionType.definition_id,
        nlohmann::json::to_cbor(legacySessionPayload));
    ck_assert_msg(legacySessionR, "create legacy session failed: %s",
                  result_message(legacySessionR));
    ck_assert_msg(store.add_edge(legacySessionR.value->ref,
                                 originalWorkspaceR.value->front().to,
                                 "workspace", "workspace", {}),
                  "link legacy session to shared workspace failed");

    auto repairedTreeR = workspace_tree(registry, store, legacySessionR.value->ref);
    ck_assert_msg(repairedTreeR, "repair legacy workspace failed: %s",
                  result_message(repairedTreeR));
    ck_assert_msg(repairedTreeR.value.value() == "Workspace\n",
                  "legacy session inherited the original session's tiles");
    auto repairedEdgesR = store.edges_from(legacySessionR.value->ref,
                                           "workspace", "workspace");
    ck_assert_msg(repairedEdgesR, "repaired workspace lookup failed: %s",
                  result_message(repairedEdgesR));
    ck_assert_uint_eq(repairedEdgesR.value->size(), 2U);
    ck_assert_msg(!(repairedEdgesR.value->back().to == originalWorkspaceR.value->front().to),
                  "legacy session retained the other session's workspace as its active root");
    auto repeatedRepairR = workspace_tree(registry, store, legacySessionR.value->ref);
    ck_assert_msg(repeatedRepairR, "repeat legacy workspace repair failed: %s",
                  result_message(repeatedRepairR));
    auto repeatedEdgesR = store.edges_from(legacySessionR.value->ref,
                                           "workspace", "workspace");
    ck_assert_msg(repeatedEdgesR, "repeated workspace lookup failed: %s",
                  result_message(repeatedEdgesR));
    ck_assert_uint_eq(repeatedEdgesR.value->size(), 2U);
    auto preservedTreeR = workspace_tree(registry, store, sessionRef);
    ck_assert_msg(preservedTreeR, "legacy repair changed original workspace lookup: %s",
                  result_message(preservedTreeR));
    ck_assert_msg(preservedTreeR.value.value() == originalTree,
                  "legacy workspace repair changed the original session tree");
    ck_assert_msg(store.close(), "close failed");
  }
  remove_db_files(path);
}
END_TEST

int main() {
  Suite* suite = suite_create("conch_layout");
  TCase* testCase = tcase_create("workspace");
  tcase_add_test(testCase, test_workspace_layout_is_ordered_nested_idempotent_and_persistent);
  suite_add_tcase(suite, testCase);
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
