extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "ceo/task_registry.h"

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

START_TEST(test_state_transitions)
{
  TaskRegistry registry;
  auto task = registry.spawn_task(referee::ObjectID::random());
  ck_assert_msg(task, "spawn failed");

  ck_assert_msg(registry.cancel_task(task.value->id), "cancel failed");
  auto canceled = registry.mark_canceled(task.value->id);
  ck_assert_msg(canceled, "mark_canceled failed");

  auto again = registry.kill_task(task.value->id);
  ck_assert_msg(!again, "expected terminal transition to fail");
}
END_TEST

START_TEST(test_invalid_parent)
{
  TaskRegistry registry;
  auto task = registry.spawn_task(referee::ObjectID::random(), 999);
  ck_assert_msg(!task, "expected invalid parent to fail");
}
END_TEST

Suite* ceo_task_suite(void) {
  Suite* s = suite_create("CeoTaskRegistry");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_spawn_and_parent_child);
  tcase_add_test(tc, test_state_transitions);
  tcase_add_test(tc, test_invalid_parent);

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
