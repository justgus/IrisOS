extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdlib>
#include <string>

using namespace referee;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

template <typename T>
std::optional<T> require_optional(const Result<std::optional<T>>& r, const char* context) {
  ck_assert_msg(r, "%s: %s", context, result_message(r));
  ck_assert_msg(r.value.has_value(), "%s: missing optional container", context);
  return r.value.value();
}

} // namespace

START_TEST(test_create_and_get_object_roundtrip)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  ck_assert_msg(store.begin(), "begin failed");

  TypeID demoType{0xABCDEF01ULL};
  ObjectID demoDef = ObjectID::random();
  auto payload = cbor_from_json_string(R"({"displayName":"Ship Propulsion"})");

  auto createdR = store.create_object(demoType, demoDef, payload);
  ck_assert_msg(createdR, "create_object failed: %s", result_message(createdR));

  auto getR = store.get_object(createdR.value->ref);
  auto opt = require_optional(getR, "get_object");
  ck_assert_msg(opt.has_value(), "expected object present");

  const auto& obj = opt.value();      // ObjectRecord
  ck_assert_uint_eq(obj.type.v, demoType.v);
  ck_assert(obj.definition_id == demoDef);

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
  ObjectID defA = ObjectID::random();
  ObjectID defB = ObjectID::random();
  auto aR = store.create_object(typeA, defA, cbor_from_json_string(R"({"a":1})"));
  auto bR = store.create_object(typeB, defB, cbor_from_json_string(R"({"b":2})"));
  ck_assert(aR);
  ck_assert(bR);

  Bytes props = cbor_from_json_string(R"({"name":"hasSubsystem"})");
  ck_assert(store.add_edge(aR.value->ref, bR.value->ref, "hasSubsystem", "subsystem", props));

  auto fromR = store.edges_from(aR.value->ref, "hasSubsystem", "subsystem");
  ck_assert(fromR);
  ck_assert_int_eq((int)fromR.value->size(), 1);
  ck_assert_str_eq(fromR.value->at(0).name.c_str(), "hasSubsystem");
  ck_assert_str_eq(fromR.value->at(0).role.c_str(), "subsystem");

  auto toR = store.edges_to(bR.value->ref, "hasSubsystem", "subsystem");
  ck_assert(toR);
  ck_assert_int_eq((int)toR.value->size(), 1);
  ck_assert_str_eq(toR.value->at(0).name.c_str(), "hasSubsystem");
  ck_assert_str_eq(toR.value->at(0).role.c_str(), "subsystem");

  ck_assert_msg(store.rollback(), "rollback failed"); // test rollback path too

  // After rollback, nothing should exist.
  auto latestA = store.get_latest(aR.value->ref.id);
  ck_assert(latestA);
  ck_assert_msg(!latestA.value->has_value(), "expected no object after rollback");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_result_carries_typed_error_code)
{
  auto invalid = Result<int>::err(ErrorCode::InvalidArgument, "bad input");
  ck_assert_msg(!invalid, "expected typed error result");
  ck_assert_msg(invalid.error.has_value(), "expected error payload");
  ck_assert(invalid.error->code == ErrorCode::InvalidArgument);
  ck_assert_str_eq(invalid.error->message.c_str(), "bad input");

  auto legacy = Result<void>::err("legacy error");
  ck_assert_msg(!legacy, "expected legacy error result");
  ck_assert_msg(legacy.error.has_value(), "expected legacy error payload");
  ck_assert(legacy.error->code == ErrorCode::Unknown);
  ck_assert_str_eq(legacy.error->message.c_str(), "legacy error");
}
END_TEST

START_TEST(test_graph_change_feed_filters_objects_and_edges)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  auto start = store.graph_cursor();
  ck_assert_msg(start, "graph_cursor failed: %s", result_message(start));

  TypeID typeA{0xA1ULL};
  TypeID typeB{0xB2ULL};
  ObjectID defA = ObjectID::random();
  ObjectID defB = ObjectID::random();
  auto aR = store.create_object(typeA, defA, cbor_from_json_string(R"({"a":1})"));
  ck_assert_msg(aR, "create a failed: %s", result_message(aR));
  auto bR = store.create_object(typeB, defB, cbor_from_json_string(R"({"b":2})"));
  ck_assert_msg(bR, "create b failed: %s", result_message(bR));

  Bytes props = cbor_from_json_string(R"({"name":"produced"})");
  auto edgeR = store.add_edge(aR.value->ref, bR.value->ref, "produced", "artifact", props);
  ck_assert_msg(edgeR, "add_edge failed: %s", result_message(edgeR));

  auto allR = store.graph_changes_after(start.value.value());
  ck_assert_msg(allR, "graph_changes_after failed: %s", result_message(allR));
  ck_assert_int_eq((int)allR.value->size(), 3);
  ck_assert(allR.value->at(0).kind == GraphChangeKind::ObjectCreated);
  ck_assert(allR.value->at(1).kind == GraphChangeKind::ObjectCreated);
  ck_assert(allR.value->at(2).kind == GraphChangeKind::EdgeCreated);

  GraphChangeFilter typeFilter;
  typeFilter.object_type = typeB;
  auto typedR = store.graph_changes_after(start.value.value(), typeFilter);
  ck_assert_msg(typedR, "typed graph_changes_after failed: %s", result_message(typedR));
  ck_assert_int_eq((int)typedR.value->size(), 1);
  ck_assert(typedR.value->at(0).kind == GraphChangeKind::ObjectCreated);
  ck_assert(typedR.value->at(0).object.has_value());
  ck_assert_uint_eq(typedR.value->at(0).object->type.v, typeB.v);

  GraphChangeFilter edgeFilter;
  edgeFilter.edge_name = "produced";
  edgeFilter.edge_role = "artifact";
  edgeFilter.edge_from = aR.value->ref;
  edgeFilter.edge_to = bR.value->ref;
  auto edgeOnlyR = store.graph_changes_after(start.value.value(), edgeFilter);
  ck_assert_msg(edgeOnlyR, "edge graph_changes_after failed: %s", result_message(edgeOnlyR));
  ck_assert_int_eq((int)edgeOnlyR.value->size(), 1);
  ck_assert(edgeOnlyR.value->at(0).kind == GraphChangeKind::EdgeCreated);
  ck_assert(edgeOnlyR.value->at(0).edge.has_value());
  ck_assert_str_eq(edgeOnlyR.value->at(0).edge->name.c_str(), "produced");

  auto end = store.graph_cursor();
  ck_assert_msg(end, "graph_cursor failed: %s", result_message(end));
  auto noneR = store.graph_changes_after(end.value.value());
  ck_assert_msg(noneR, "graph_changes_after after end failed: %s", result_message(noneR));
  ck_assert_int_eq((int)noneR.value->size(), 0);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* referee_suite(void) {
  Suite* s = suite_create("RefereeCore");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_create_and_get_object_roundtrip);
  tcase_add_test(tc, test_edges_from_and_to);
  tcase_add_test(tc, test_result_carries_typed_error_code);
  tcase_add_test(tc, test_graph_change_feed_filters_objects_and_edges);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  setenv("CK_FORK", "no", 1);
  Suite* s = referee_suite();
  SRunner* sr = srunner_create(s);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
