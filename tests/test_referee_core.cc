extern "C" {
#include <check.h>
}

#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <string>

using namespace referee;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

} // namespace

START_TEST(test_create_and_get_object_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  ck_assert_msg(store.begin(), "begin failed");

  TypeID demoType{0xABCDEF01ULL};
  auto payload = cbor_from_json_string(R"({"displayName":"Ship Propulsion"})");

  auto createdR = store.create_object(demoType, payload);
  ck_assert_msg(createdR, "create_object failed: %s", result_message(createdR));

  auto getR = store.get_object(createdR.value->ref);
  ck_assert_msg(getR, "get_object failed: %s", result_message(getR));
  ck_assert_msg(getR.value->has_value(), "expected object present");

  auto& opt = getR.value.value();     // inner optional<ObjectRecord>
  ck_assert_msg(opt.has_value(), "expected object present");

  const auto& obj = opt.value();      // ObjectRecord
  ck_assert_uint_eq(obj.type.v, demoType.v);

  auto json = json_string_from_cbor(obj.payload_cbor);
  ck_assert_msg(json.find("Ship Propulsion") != std::string::npos, "payload didn't match json=%s", json.c_str());

  ck_assert_msg(store.commit(), "commit failed");
  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_edges_from_and_to)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  ck_assert_msg(store.begin(), "begin failed");

  TypeID typeA{1}, typeB{2};
  auto aR = store.create_object(typeA, cbor_from_json_string(R"({"a":1})"));
  auto bR = store.create_object(typeB, cbor_from_json_string(R"({"b":2})"));
  ck_assert(aR);
  ck_assert(bR);

  Bytes props = cbor_from_json_string(R"({"name":"hasSubsystem"})");
  ck_assert(store.add_edge(aR.value->ref, bR.value->ref, "hasSubsystem", props));

  auto fromR = store.edges_from(aR.value->ref);
  ck_assert(fromR);
  ck_assert_int_eq((int)fromR.value->size(), 1);
  ck_assert_str_eq(fromR.value->at(0).name.c_str(), "hasSubsystem");

  auto toR = store.edges_to(bR.value->ref);
  ck_assert(toR);
  ck_assert_int_eq((int)toR.value->size(), 1);
  ck_assert_str_eq(toR.value->at(0).name.c_str(), "hasSubsystem");

  ck_assert_msg(store.rollback(), "rollback failed"); // test rollback path too

  // After rollback, nothing should exist.
  auto latestA = store.get_latest(aR.value->ref.id);
  ck_assert(latestA);
  ck_assert_msg(!latestA.value->has_value(), "expected no object after rollback");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* referee_suite(void) {
  Suite* s = suite_create("RefereeCore");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_create_and_get_object_roundtrip);
  tcase_add_test(tc, test_edges_from_and_to);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = referee_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
