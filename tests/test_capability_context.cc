extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "services/capability_context.h"

#include <cstdio>
#include <string>
#include <unistd.h>

using namespace iris::service;
using namespace referee;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

std::string make_temp_db_path() {
  char tmpl[] = "/tmp/iris_capability_XXXXXX";
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

START_TEST(test_capability_context_roundtrip_and_subject_lookup)
{
  std::string db_path = make_temp_db_path();
  ObjectID context_id = ObjectID::random();
  ObjectID subject_id = ObjectID::random();
  ObjectID sandbox_id = ObjectID::random();

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    CapabilityContext context;
    context.id = context_id;
    context.subject = subject_id;
    context.sandbox = sandbox_id;
    context.grants.push_back(CapabilityGrant{"service.registry.read"});
    context.grants.push_back(CapabilityGrant{"service.lifecycle.start"});

    CapabilityContextStore contexts(store);
    auto savedR = contexts.persist_context(context);
    ck_assert_msg(savedR, "persist_context failed: %s", result_message(savedR));
    ck_assert(savedR.value->context.id == context_id);
    ck_assert(savedR.value->context.subject == subject_id);
    ck_assert(savedR.value->context.sandbox.has_value());
    ck_assert(savedR.value->context.sandbox.value() == sandbox_id);

    ck_assert_msg(store.close(), "close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    CapabilityContextStore contexts(store);
    auto loadedR = contexts.get_context(context_id);
    ck_assert_msg(loadedR, "get_context failed: %s", result_message(loadedR));
    ck_assert_msg(loadedR.value->has_value(), "expected persisted capability context");
    ck_assert(loadedR.value->value().context.subject == subject_id);
    ck_assert_uint_eq((unsigned int)loadedR.value->value().context.grants.size(), 2U);
    ck_assert_str_eq(loadedR.value->value().context.grants[0].name.c_str(),
                     "service.lifecycle.start");
    ck_assert_str_eq(loadedR.value->value().context.grants[1].name.c_str(),
                     "service.registry.read");

    auto by_subjectR = contexts.list_contexts_for_subject(subject_id);
    ck_assert_msg(by_subjectR, "list_contexts_for_subject failed: %s", result_message(by_subjectR));
    ck_assert_uint_eq((unsigned int)by_subjectR.value->size(), 1U);
    ck_assert(by_subjectR.value->front().context.id == context_id);

    auto other_subjectR = contexts.list_contexts_for_subject(ObjectID::random());
    ck_assert_msg(other_subjectR, "other subject lookup failed: %s", result_message(other_subjectR));
    ck_assert_msg(other_subjectR.value->empty(), "expected no contexts for other subject");

    ck_assert_msg(store.close(), "close failed");
  }

  cleanup_db_files(db_path);
}
END_TEST

START_TEST(test_capability_context_rejects_invalid_grants)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  CapabilityContextStore contexts(store);
  CapabilityContext empty_name;
  empty_name.id = ObjectID::random();
  empty_name.subject = ObjectID::random();
  empty_name.grants.push_back(CapabilityGrant{""});

  auto emptyR = contexts.persist_context(empty_name);
  ck_assert_msg(!emptyR, "expected empty capability name to fail");
  ck_assert_msg(emptyR.error.has_value(), "expected error details");
  ck_assert_int_eq((int)emptyR.error->code, (int)ErrorCode::InvalidArgument);

  CapabilityContext duplicate;
  duplicate.id = ObjectID::random();
  duplicate.subject = ObjectID::random();
  duplicate.grants.push_back(CapabilityGrant{"service.registry.read"});
  duplicate.grants.push_back(CapabilityGrant{"service.registry.read"});

  auto duplicateR = contexts.persist_context(duplicate);
  ck_assert_msg(!duplicateR, "expected duplicate capability name to fail");
  ck_assert_msg(duplicateR.error.has_value(), "expected error details");
  ck_assert_int_eq((int)duplicateR.error->code, (int)ErrorCode::InvalidArgument);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_sandbox_identity_roundtrip_and_subject_lookup)
{
  std::string db_path = make_temp_db_path();
  ObjectID sandbox_id = ObjectID::random();
  ObjectID subject_a = ObjectID::random();
  ObjectID subject_b = ObjectID::random();

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    SandboxIdentity sandbox;
    sandbox.id = sandbox_id;
    sandbox.name = "service-host-sandbox";
    sandbox.subjects.push_back(subject_b);
    sandbox.subjects.push_back(subject_a);

    CapabilityContextStore contexts(store);
    auto savedR = contexts.persist_sandbox(sandbox);
    ck_assert_msg(savedR, "persist_sandbox failed: %s", result_message(savedR));
    ck_assert(savedR.value->sandbox.id == sandbox_id);
    ck_assert_str_eq(savedR.value->sandbox.name.c_str(), "service-host-sandbox");
    ck_assert_uint_eq((unsigned int)savedR.value->sandbox.subjects.size(), 2U);
    ck_assert_msg(savedR.value->sandbox.subjects[0].to_hex() <
                  savedR.value->sandbox.subjects[1].to_hex(),
                  "expected deterministic subject ordering");
    ck_assert_msg((savedR.value->sandbox.subjects[0] == subject_a ||
                   savedR.value->sandbox.subjects[1] == subject_a),
                  "expected subject_a in sandbox subjects");
    ck_assert_msg((savedR.value->sandbox.subjects[0] == subject_b ||
                   savedR.value->sandbox.subjects[1] == subject_b),
                  "expected subject_b in sandbox subjects");

    ck_assert_msg(store.close(), "close failed");
  }

  {
    SqliteStore store(SqliteConfig{ .filename=db_path, .enable_wal=true });
    ck_assert_msg(store.open(), "open failed");
    ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

    CapabilityContextStore contexts(store);
    auto loadedR = contexts.get_sandbox(sandbox_id);
    ck_assert_msg(loadedR, "get_sandbox failed: %s", result_message(loadedR));
    ck_assert_msg(loadedR.value->has_value(), "expected persisted sandbox identity");
    ck_assert_str_eq(loadedR.value->value().sandbox.name.c_str(), "service-host-sandbox");

    auto by_subjectR = contexts.list_sandboxes_for_subject(subject_a);
    ck_assert_msg(by_subjectR, "list_sandboxes_for_subject failed: %s", result_message(by_subjectR));
    ck_assert_uint_eq((unsigned int)by_subjectR.value->size(), 1U);
    ck_assert(by_subjectR.value->front().sandbox.id == sandbox_id);

    auto other_subjectR = contexts.list_sandboxes_for_subject(ObjectID::random());
    ck_assert_msg(other_subjectR, "other subject lookup failed: %s", result_message(other_subjectR));
    ck_assert_msg(other_subjectR.value->empty(), "expected no sandboxes for other subject");

    ck_assert_msg(store.close(), "close failed");
  }

  cleanup_db_files(db_path);
}
END_TEST

START_TEST(test_sandbox_identity_rejects_invalid_metadata)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  CapabilityContextStore contexts(store);
  SandboxIdentity empty_name;
  empty_name.id = ObjectID::random();

  auto emptyR = contexts.persist_sandbox(empty_name);
  ck_assert_msg(!emptyR, "expected empty sandbox name to fail");
  ck_assert_msg(emptyR.error.has_value(), "expected error details");
  ck_assert_int_eq((int)emptyR.error->code, (int)ErrorCode::InvalidArgument);

  SandboxIdentity duplicate;
  duplicate.id = ObjectID::random();
  duplicate.name = "duplicate-subjects";
  auto subject = ObjectID::random();
  duplicate.subjects.push_back(subject);
  duplicate.subjects.push_back(subject);

  auto duplicateR = contexts.persist_sandbox(duplicate);
  ck_assert_msg(!duplicateR, "expected duplicate sandbox subject to fail");
  ck_assert_msg(duplicateR.error.has_value(), "expected error details");
  ck_assert_int_eq((int)duplicateR.error->code, (int)ErrorCode::InvalidArgument);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* capability_context_suite(void) {
  Suite* s = suite_create("CapabilityContext");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_capability_context_roundtrip_and_subject_lookup);
  tcase_add_test(tc, test_capability_context_rejects_invalid_grants);
  tcase_add_test(tc, test_sandbox_identity_roundtrip_and_subject_lookup);
  tcase_add_test(tc, test_sandbox_identity_rejects_invalid_metadata);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = capability_context_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
