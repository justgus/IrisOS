extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "ceo/task_registry.h"
#include "exec/await.h"
#include "exec/waitables.h"

using namespace iris::ceo;
using namespace iris::exec;

START_TEST(test_await_and_cancel)
{
  TaskRegistry registry;
  Event ev(false);

  auto task = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task, "spawn failed");

  auto waitR = await_task(ev, registry, task.value->id);
  ck_assert_msg(waitR, "await_task failed");
  ck_assert_msg(!waitR.value->ready, "expected await to block");

  ck_assert_msg(registry.cancel_task(task.value->id), "cancel_task failed");

  auto signal = ev.signal();
  auto outcome = handle_wait_result(registry, signal);
  ck_assert_int_eq((int)outcome.canceled.size(), 1);

  auto state = registry.get_task(task.value->id);
  ck_assert_msg(state, "get_task failed");
  ck_assert(state.value->has_value());
  ck_assert_int_eq((int)state.value->value().state, (int)TaskState::Canceled);
}
END_TEST

START_TEST(test_await_resume)
{
  TaskRegistry registry;
  Event ev(false);

  auto task = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task, "spawn failed");

  auto waitR = await_task(ev, registry, task.value->id);
  ck_assert_msg(waitR, "await_task failed");
  ck_assert_msg(!waitR.value->ready, "expected await to block");

  auto signal = ev.signal();
  auto outcome = handle_wait_result(registry, signal);
  ck_assert_int_eq((int)outcome.resumed.size(), 1);

  auto state = registry.get_task(task.value->id);
  ck_assert_msg(state, "get_task failed");
  ck_assert(state.value->has_value());
  ck_assert_int_eq((int)state.value->value().state, (int)TaskState::Running);
}
END_TEST

Suite* exec_integration_suite(void) {
  Suite* s = suite_create("ExecIntegration");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_await_and_cancel);
  tcase_add_test(tc, test_await_resume);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = exec_integration_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
