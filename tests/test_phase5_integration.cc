extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "ceo/io_reactor.h"
#include "comms/primitives.h"

using namespace iris::ceo;
using namespace iris::comms;
using namespace iris::exec;

namespace {

const char* result_message(const referee::Result<WaitResult>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

} // namespace

START_TEST(test_phase5_bytestream_reactor_roundtrip)
{
  TaskRegistry registry;
  IoReactor reactor(registry);

  auto taskR = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(taskR, "spawn_task failed");

  ByteStream stream;

  auto waitR = reactor.await_readable(stream, taskR.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(!waitR.value->ready, "expected initial wait to block");

  auto taskWaitingR = registry.get_task(taskR.value->id);
  ck_assert_msg(taskWaitingR, "get_task failed");
  ck_assert_msg(taskWaitingR.value->has_value(), "expected task record");
  ck_assert_int_eq((int)taskWaitingR.value->value().state, (int)TaskState::Waiting);

  Bytes payload = {0xAA, 0xBB, 0xCC};
  auto outcome = reactor.push(stream, payload);
  ck_assert_uint_eq((unsigned int)outcome.resumed.size(), 1U);
  ck_assert_uint_eq((unsigned int)outcome.resumed[0], (unsigned int)taskR.value->id);

  auto taskRunningR = registry.get_task(taskR.value->id);
  ck_assert_msg(taskRunningR, "get_task failed");
  ck_assert_msg(taskRunningR.value->has_value(), "expected task record");
  ck_assert_int_eq((int)taskRunningR.value->value().state, (int)TaskState::Running);

  auto recv = stream.recv(10);
  ck_assert_uint_eq((unsigned int)recv.size(), 3U);
  ck_assert_uint_eq(recv[0], 0xAA);
  ck_assert_uint_eq(recv[2], 0xCC);
}
END_TEST

START_TEST(test_phase5_bytestream_ready_before_wait)
{
  TaskRegistry registry;
  IoReactor reactor(registry);

  auto taskR = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(taskR, "spawn_task failed");

  ByteStream stream;
  Bytes payload = {0x11};
  reactor.push(stream, payload);

  auto waitR = reactor.await_readable(stream, taskR.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(waitR.value->ready, "expected wait to be ready when data present");

  auto taskR2 = registry.get_task(taskR.value->id);
  ck_assert_msg(taskR2, "get_task failed");
  ck_assert_msg(taskR2.value->has_value(), "expected task record");
  ck_assert_int_eq((int)taskR2.value->value().state, (int)TaskState::Running);

  auto recv = stream.recv(1);
  ck_assert_uint_eq((unsigned int)recv.size(), 1U);
  ck_assert_uint_eq(recv[0], 0x11);
}
END_TEST

Suite* phase5_integration_suite(void) {
  Suite* s = suite_create("Phase5Integration");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_phase5_bytestream_reactor_roundtrip);
  tcase_add_test(tc, test_phase5_bytestream_ready_before_wait);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = phase5_integration_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
