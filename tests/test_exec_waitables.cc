extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "exec/waitables.h"

using namespace iris::exec;

START_TEST(test_event_wait_signal)
{
  Event ev(false);
  auto waitR = ev.wait(1);
  ck_assert_msg(!waitR.ready, "event should block");

  auto sig = ev.signal();
  ck_assert_msg(sig.ready, "signal should be ready");
  ck_assert_int_eq((int)sig.woken.size(), 1);
}
END_TEST

START_TEST(test_semaphore_basic)
{
  Semaphore sem(1);
  auto first = sem.wait(1);
  ck_assert_msg(first.ready, "first wait should be ready");

  auto second = sem.wait(2);
  ck_assert_msg(!second.ready, "second wait should block");

  auto sig = sem.signal(1);
  ck_assert_int_eq((int)sig.woken.size(), 1);
}
END_TEST

START_TEST(test_mutex_lock_unlock)
{
  Mutex mtx;
  auto a = mtx.wait(1);
  ck_assert_msg(a.ready, "first lock should succeed");

  auto b = mtx.wait(2);
  ck_assert_msg(!b.ready, "second lock should block");

  auto u = mtx.unlock(1);
  ck_assert_msg(u.ready, "unlock should succeed");
  ck_assert_int_eq((int)u.woken.size(), 1);
}
END_TEST

START_TEST(test_future_set)
{
  Future fut;
  auto waitR = fut.wait(5);
  ck_assert_msg(!waitR.ready, "future should block");

  auto set = fut.set_value("ok");
  ck_assert_msg(set.ready, "set should be ready");
  ck_assert_int_eq((int)set.woken.size(), 1);
}
END_TEST

Suite* exec_waitables_suite(void) {
  Suite* s = suite_create("ExecWaitables");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_event_wait_signal);
  tcase_add_test(tc, test_semaphore_basic);
  tcase_add_test(tc, test_mutex_lock_unlock);
  tcase_add_test(tc, test_future_set);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = exec_waitables_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
