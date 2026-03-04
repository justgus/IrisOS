extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "ceo/task_registry.h"
#include "comms/primitives.h"

using namespace iris::ceo;

START_TEST(test_spawn_and_parent_child)
{
  TaskRegistry registry;
  auto root = registry.spawn_task(referee::ObjectID::random(), std::nullopt, "root");
  ck_assert_msg(root, "spawn root failed");
  ck_assert_int_eq((int)root.value->state, (int)TaskState::Running);

  auto child = registry.spawn_task(referee::ObjectID::random(), root.value->id, "child");
  ck_assert_msg(child, "spawn child failed");
  ck_assert(child.value->parent.has_value());
  ck_assert_uint_eq(child.value->parent.value(), root.value->id);

  auto root_lookup = registry.get_task(root.value->id);
  ck_assert_msg(root_lookup, "get_task failed");
  ck_assert(root_lookup.value->has_value());
  ck_assert_int_eq((int)root_lookup.value->value().children.size(), 1);
}
END_TEST

START_TEST(test_cancel_propagation_owned_only)
{
  TaskRegistry registry;
  auto root = registry.spawn_task(referee::ObjectID::random(), std::nullopt, "root");
  ck_assert_msg(root, "spawn root failed");

  auto owned = registry.spawn_task(referee::ObjectID::random(), root.value->id, "owned");
  ck_assert_msg(owned, "spawn owned child failed");

  auto service = registry.spawn_task(referee::ObjectID::random(), root.value->id, "service",
                                     TaskMode::Service);
  ck_assert_msg(service, "spawn service child failed");

  auto detached_owned = registry.spawn_task(referee::ObjectID::random(), root.value->id, "detached",
                                            TaskMode::Inline, ChildOwnership::Detached);
  ck_assert_msg(detached_owned, "spawn detached child failed");

  ck_assert_msg(registry.cancel_task(root.value->id), "cancel root failed");

  auto owned_lookup = registry.get_task(owned.value->id);
  ck_assert_msg(owned_lookup, "owned lookup failed");
  ck_assert_int_eq((int)owned_lookup.value->value().state, (int)TaskState::CancelRequested);

  auto service_lookup = registry.get_task(service.value->id);
  ck_assert_msg(service_lookup, "service lookup failed");
  ck_assert_int_eq((int)service_lookup.value->value().state, (int)TaskState::Running);

  auto detached_lookup = registry.get_task(detached_owned.value->id);
  ck_assert_msg(detached_lookup, "detached lookup failed");
  ck_assert_int_eq((int)detached_lookup.value->value().state, (int)TaskState::Running);
}
END_TEST

START_TEST(test_state_transitions)
{
  TaskRegistry registry;
  auto task = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task, "spawn failed");

  auto resume_before_wait = registry.resume_task(task.value->id);
  ck_assert_msg(!resume_before_wait, "expected invalid resume to fail");

  ck_assert_msg(registry.cancel_task(task.value->id), "cancel failed");
  auto canceled = registry.mark_canceled(task.value->id);
  ck_assert_msg(canceled, "mark_canceled failed");

  auto again = registry.kill_task(task.value->id);
  ck_assert_msg(!again, "expected terminal transition to fail");
}
END_TEST

START_TEST(test_wait_and_resume)
{
  TaskRegistry registry;
  auto task = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task, "spawn failed");

  auto waitR = registry.wait_task(task.value->id);
  ck_assert_msg(waitR, "wait_task failed");

  auto resumeR = registry.resume_task(task.value->id);
  ck_assert_msg(resumeR, "resume_task failed");
}
END_TEST

START_TEST(test_invalid_parent)
{
  TaskRegistry registry;
  auto task = registry.spawn_task(referee::ObjectID::random(), 999);
  ck_assert_msg(!task, "expected invalid parent to fail");
}
END_TEST

START_TEST(test_create_start_stop)
{
  TaskRegistry registry;
  auto task = registry.create_task(referee::ObjectID::random());
  ck_assert_msg(task, "create failed");
  ck_assert_int_eq((int)task.value->state, (int)TaskState::Created);

  ck_assert_msg(registry.start_task(task.value->id), "start_task failed");
  auto running = registry.get_task(task.value->id);
  ck_assert_msg(running, "get_task failed");
  ck_assert_int_eq((int)running.value->value().state, (int)TaskState::Running);

  ck_assert_msg(registry.stop_task(task.value->id), "stop_task failed");
  auto stopped = registry.get_task(task.value->id);
  ck_assert_msg(stopped, "get_task failed");
  ck_assert_int_eq((int)stopped.value->value().state, (int)TaskState::CancelRequested);
}
END_TEST

START_TEST(test_task_comms_open_close)
{
  TaskRegistry registry;
  TaskComms comms(registry);

  auto a = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(a, "spawn a failed");
  auto b = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(b, "spawn b failed");

  auto channelR = comms.open_channel(a.value->id, b.value->id);
  ck_assert_msg(channelR, "open_channel failed");
  comms.close_channel(channelR.value->first);
  ck_assert_msg(channelR.value->first.closed(), "channel should be closed");
  ck_assert_msg(channelR.value->second.closed(), "peer channel should be closed");

  auto datagramR = comms.open_datagram(a.value->id, b.value->id);
  ck_assert_msg(datagramR, "open_datagram failed");
  comms.close_datagram(datagramR.value->first);
  ck_assert_msg(datagramR.value->first.closed(), "datagram should be closed");
  ck_assert_msg(datagramR.value->second.closed(), "peer datagram should be closed");
}
END_TEST

Suite* ceo_task_suite(void) {
  Suite* s = suite_create("CeoTaskRegistry");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_spawn_and_parent_child);
  tcase_add_test(tc, test_cancel_propagation_owned_only);
  tcase_add_test(tc, test_state_transitions);
  tcase_add_test(tc, test_wait_and_resume);
  tcase_add_test(tc, test_invalid_parent);
  tcase_add_test(tc, test_create_start_stop);
  tcase_add_test(tc, test_task_comms_open_close);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = ceo_task_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
