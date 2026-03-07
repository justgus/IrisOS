extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "ceo/io_reactor.h"
#include "comms/primitives.h"
#include "refract/bootstrap.h"
#include "refract/dispatch.h"
#include "referee_sqlite/sqlite_store.h"

using namespace iris::ceo;
using namespace iris::comms;
using namespace iris::exec;
using namespace iris::refract;

namespace {

const char* result_message(const referee::Result<WaitResult>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

template <typename T>
const char* result_message(const referee::Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

constexpr referee::TypeID kTypeU64{0x1002ULL};
constexpr referee::TypeID kTypeBytes{0x1007ULL};
constexpr referee::TypeID kTypeKernelIo{0x4B494F5000000001ULL};
constexpr referee::TypeID kTypeKernelIoChannel{0x4B494F5000000002ULL};
constexpr referee::TypeID kTypeKernelIoDatagram{0x4B494F5000000003ULL};

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

START_TEST(test_conduit_channel_flow)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry schema(store);
  auto boot = bootstrap_core_schema(schema);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TaskRegistry registry;
  TaskComms comms(registry);
  IoReactor reactor(registry);
  iris::conduit::IoHandleStore handles;
  iris::conduit::IoExecutor executor(registry, comms, reactor, handles);

  auto task_a = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_a, "spawn task_a failed");
  auto task_b = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_b, "spawn task_b failed");

  DispatchEngine engine(schema);
  auto open_match = engine.resolve(kTypeKernelIo, "open_channel", OperationScope::Class,
                                   {kTypeU64, kTypeU64}, 2, true);
  ck_assert_msg(open_match, "open_channel resolve failed: %s", result_message(open_match));
  auto openR = executor.open_channel(open_match.value.value(), task_a.value->id, task_b.value->id);
  ck_assert_msg(openR, "open_channel failed: %s", result_message(openR));

  auto await_match = engine.resolve(kTypeKernelIoChannel, "await_readable", OperationScope::Object,
                                    {kTypeU64}, 1, true);
  ck_assert_msg(await_match, "await_readable resolve failed: %s", result_message(await_match));
  auto waitR = executor.await_channel(await_match.value.value(), openR.value->second, task_b.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(!waitR.value->ready, "expected initial wait to block");

  auto task_waiting = registry.get_task(task_b.value->id);
  ck_assert_msg(task_waiting, "get_task failed");
  ck_assert_msg(task_waiting.value->has_value(), "expected task record");
  ck_assert_int_eq((int)task_waiting.value->value().state, (int)TaskState::Waiting);

  auto send_match = engine.resolve(kTypeKernelIoChannel, "send", OperationScope::Object,
                                   {kTypeBytes}, 1, true);
  ck_assert_msg(send_match, "send resolve failed: %s", result_message(send_match));
  Bytes payload = {0x01, 0x02, 0x03};
  auto sendR = executor.send_channel(send_match.value.value(), openR.value->first, payload);
  ck_assert_msg(sendR, "send failed: %s", result_message(sendR));
  ck_assert_msg(sendR.value->ready, "expected send ready");
  ck_assert_uint_eq((unsigned int)sendR.value->outcome.resumed.size(), 1U);
  ck_assert_uint_eq((unsigned int)sendR.value->outcome.resumed[0], (unsigned int)task_b.value->id);

  auto recv_match = engine.resolve(kTypeKernelIoChannel, "recv", OperationScope::Object,
                                   {kTypeU64}, 1, true);
  ck_assert_msg(recv_match, "recv resolve failed: %s", result_message(recv_match));
  auto recvR = executor.recv_channel(recv_match.value.value(), openR.value->second, 8);
  ck_assert_msg(recvR, "recv failed: %s", result_message(recvR));
  ck_assert_uint_eq((unsigned int)recvR.value->size(), 3U);

  auto close_match = engine.resolve(kTypeKernelIoChannel, "close", OperationScope::Object, {}, 0, true);
  ck_assert_msg(close_match, "close resolve failed: %s", result_message(close_match));
  ck_assert_msg(executor.close_channel(close_match.value.value(), openR.value->second),
                "close failed");

  auto* sender = handles.find_channel(openR.value->first);
  ck_assert_msg(sender != nullptr, "expected remaining channel handle");
  ck_assert_msg(sender->closed(), "expected channel closed after peer close");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_conduit_datagram_flow)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry schema(store);
  auto boot = bootstrap_core_schema(schema);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TaskRegistry registry;
  TaskComms comms(registry);
  IoReactor reactor(registry);
  iris::conduit::IoHandleStore handles;
  iris::conduit::IoExecutor executor(registry, comms, reactor, handles);

  auto task_a = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_a, "spawn task_a failed");
  auto task_b = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_b, "spawn task_b failed");

  DispatchEngine engine(schema);
  auto open_match = engine.resolve(kTypeKernelIo, "open_datagram", OperationScope::Class,
                                   {kTypeU64, kTypeU64}, 2, true);
  ck_assert_msg(open_match, "open_datagram resolve failed: %s", result_message(open_match));
  auto openR = executor.open_datagram(open_match.value.value(), task_a.value->id, task_b.value->id);
  ck_assert_msg(openR, "open_datagram failed: %s", result_message(openR));

  auto await_match = engine.resolve(kTypeKernelIoDatagram, "await_readable", OperationScope::Object,
                                    {kTypeU64}, 1, true);
  ck_assert_msg(await_match, "await_readable resolve failed: %s", result_message(await_match));
  auto waitR = executor.await_datagram(await_match.value.value(), openR.value->second, task_b.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(!waitR.value->ready, "expected initial wait to block");

  auto send_match = engine.resolve(kTypeKernelIoDatagram, "send", OperationScope::Object,
                                   {kTypeBytes}, 1, true);
  ck_assert_msg(send_match, "send resolve failed: %s", result_message(send_match));
  Bytes payload = {0x10, 0x20};
  auto sendR = executor.send_datagram(send_match.value.value(), openR.value->first, payload);
  ck_assert_msg(sendR, "send failed: %s", result_message(sendR));
  ck_assert_msg(sendR.value->ready, "expected send ready");

  auto recv_match = engine.resolve(kTypeKernelIoDatagram, "recv", OperationScope::Object, {}, 0, true);
  ck_assert_msg(recv_match, "recv resolve failed: %s", result_message(recv_match));
  auto recvR = executor.recv_datagram(recv_match.value.value(), openR.value->second);
  ck_assert_msg(recvR, "recv failed: %s", result_message(recvR));
  ck_assert_msg(recvR.value->has_value(), "expected datagram payload");
  ck_assert_uint_eq((unsigned int)recvR.value->value().size(), 2U);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_conduit_invalid_handle)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry schema(store);
  auto boot = bootstrap_core_schema(schema);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TaskRegistry registry;
  TaskComms comms(registry);
  IoReactor reactor(registry);
  iris::conduit::IoHandleStore handles;
  iris::conduit::IoExecutor executor(registry, comms, reactor, handles);

  DispatchEngine engine(schema);
  auto send_match = engine.resolve(kTypeKernelIoChannel, "send", OperationScope::Object,
                                   {kTypeBytes}, 1, true);
  ck_assert_msg(send_match, "send resolve failed: %s", result_message(send_match));

  iris::conduit::IoHandle bad_handle{iris::conduit::IoHandleKind::Channel, 999};
  Bytes payload = {0x01};
  auto sendR = executor.send_channel(send_match.value.value(), bad_handle, payload);
  ck_assert_msg(!sendR, "expected invalid handle to fail");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_conduit_await_cancel)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry schema(store);
  auto boot = bootstrap_core_schema(schema);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TaskRegistry registry;
  TaskComms comms(registry);
  IoReactor reactor(registry);
  iris::conduit::IoHandleStore handles;
  iris::conduit::IoExecutor executor(registry, comms, reactor, handles);

  auto task_a = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_a, "spawn task_a failed");
  auto task_b = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_b, "spawn task_b failed");

  DispatchEngine engine(schema);
  auto open_match = engine.resolve(kTypeKernelIo, "open_channel", OperationScope::Class,
                                   {kTypeU64, kTypeU64}, 2, true);
  ck_assert_msg(open_match, "open_channel resolve failed: %s", result_message(open_match));
  auto openR = executor.open_channel(open_match.value.value(), task_a.value->id, task_b.value->id);
  ck_assert_msg(openR, "open_channel failed: %s", result_message(openR));

  ck_assert_msg(registry.cancel_task(task_b.value->id), "cancel failed");

  auto await_match = engine.resolve(kTypeKernelIoChannel, "await_readable", OperationScope::Object,
                                    {kTypeU64}, 1, true);
  ck_assert_msg(await_match, "await_readable resolve failed: %s", result_message(await_match));
  auto waitR = executor.await_channel(await_match.value.value(), openR.value->second, task_b.value->id);
  ck_assert_msg(waitR, "await_readable failed: %s", result_message(waitR));
  ck_assert_msg(waitR.value->ready, "expected await to be ready due to cancel");
  auto canceled = registry.get_task(task_b.value->id);
  ck_assert_msg(canceled, "get_task failed");
  ck_assert_msg(canceled.value->has_value(), "expected task record");
  ck_assert_int_eq((int)canceled.value->value().state, (int)TaskState::Canceled);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_conduit_channel_close_errors)
{
  referee::SqliteStore store(referee::SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  SchemaRegistry schema(store);
  auto boot = bootstrap_core_schema(schema);
  ck_assert_msg(boot, "bootstrap failed: %s", result_message(boot));

  TaskRegistry registry;
  TaskComms comms(registry);
  IoReactor reactor(registry);
  iris::conduit::IoHandleStore handles;
  iris::conduit::IoExecutor executor(registry, comms, reactor, handles);

  auto task_a = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_a, "spawn task_a failed");
  auto task_b = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task_b, "spawn task_b failed");

  DispatchEngine engine(schema);
  auto open_match = engine.resolve(kTypeKernelIo, "open_channel", OperationScope::Class,
                                   {kTypeU64, kTypeU64}, 2, true);
  ck_assert_msg(open_match, "open_channel resolve failed: %s", result_message(open_match));
  auto openR = executor.open_channel(open_match.value.value(), task_a.value->id, task_b.value->id);
  ck_assert_msg(openR, "open_channel failed: %s", result_message(openR));

  auto close_match = engine.resolve(kTypeKernelIoChannel, "close", OperationScope::Object, {}, 0, true);
  ck_assert_msg(close_match, "close resolve failed: %s", result_message(close_match));

  ck_assert_msg(executor.close_channel(close_match.value.value(), openR.value->first),
                "close failed");

  auto close_again = executor.close_channel(close_match.value.value(), openR.value->first);
  ck_assert_msg(!close_again, "expected close of missing handle to fail");

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

Suite* ceo_io_reactor_suite(void) {
  Suite* s = suite_create("CeoIoReactor");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_reactor_wakes_channel_waiter);
  tcase_add_test(tc, test_reactor_wakes_datagram_waiter);
  tcase_add_test(tc, test_conduit_channel_flow);
  tcase_add_test(tc, test_conduit_datagram_flow);
  tcase_add_test(tc, test_conduit_invalid_handle);
  tcase_add_test(tc, test_conduit_await_cancel);
  tcase_add_test(tc, test_conduit_channel_close_errors);

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
