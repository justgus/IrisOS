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

START_TEST(test_reactor_wakes_channel_waiter)
{
  TaskRegistry registry;
  IoReactor reactor(registry);

  auto taskR = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(taskR, "spawn_task failed");

  auto pair = Channel::loopback();
  Channel& sender = pair.first;
  Channel& receiver = pair.second;

  auto waitR = reactor.await_readable(receiver, taskR.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(!waitR.value->ready, "expected initial wait to block");

  auto taskWaitingR = registry.get_task(taskR.value->id);
  ck_assert_msg(taskWaitingR, "get_task failed");
  ck_assert_msg(taskWaitingR.value->has_value(), "expected task record");
  ck_assert_int_eq((int)taskWaitingR.value->value().state, (int)TaskState::Waiting);

  Bytes payload = {0x01, 0x02, 0x03};
  auto outcome = reactor.send(sender, payload);
  ck_assert_uint_eq((unsigned int)outcome.resumed.size(), 1U);
  ck_assert_uint_eq((unsigned int)outcome.resumed[0], (unsigned int)taskR.value->id);

  auto taskRunningR = registry.get_task(taskR.value->id);
  ck_assert_msg(taskRunningR, "get_task failed");
  ck_assert_msg(taskRunningR.value->has_value(), "expected task record");
  ck_assert_int_eq((int)taskRunningR.value->value().state, (int)TaskState::Running);

  auto recv = receiver.recv(10);
  ck_assert_uint_eq((unsigned int)recv.size(), 3U);
}
END_TEST

START_TEST(test_reactor_wakes_datagram_waiter)
{
  TaskRegistry registry;
  IoReactor reactor(registry);

  auto taskR = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(taskR, "spawn_task failed");

  auto pair = DatagramPort::loopback();
  DatagramPort& sender = pair.first;
  DatagramPort& receiver = pair.second;

  auto waitR = reactor.await_readable(receiver, taskR.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(!waitR.value->ready, "expected initial wait to block");

  Bytes packet = {0x10, 0x20};
  auto outcome = reactor.send(sender, packet);
  ck_assert_uint_eq((unsigned int)outcome.resumed.size(), 1U);
  ck_assert_uint_eq((unsigned int)outcome.resumed[0], (unsigned int)taskR.value->id);

  auto recv = receiver.recv();
  ck_assert_msg(recv.has_value(), "expected datagram payload");
  ck_assert_uint_eq((unsigned int)recv->size(), 2U);
}
END_TEST

Suite* ceo_io_reactor_suite(void) {
  Suite* s = suite_create("CeoIoReactor");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_reactor_wakes_channel_waiter);
  tcase_add_test(tc, test_reactor_wakes_datagram_waiter);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = ceo_io_reactor_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
