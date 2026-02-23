extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "comms/primitives.h"

#include <cstdint>

using namespace iris::comms;

START_TEST(test_channel_loopback_roundtrip)
{
  auto pair = Channel::loopback();
  Channel& a = pair.first;
  Channel& b = pair.second;

  exec::WaitResult wait_before = b.wait_readable(1);
  ck_assert_msg(!wait_before.ready, "expected no data before send");

  Bytes payload = {0x10, 0x20, 0x30, 0x40};
  auto sendR = a.send(payload);
  ck_assert_msg(sendR.ready, "send should mark readable");

  exec::WaitResult wait_after = b.wait_readable(2);
  ck_assert_msg(wait_after.ready, "expected data after send");

  auto recv = b.recv(10);
  ck_assert_uint_eq((unsigned int)recv.size(), 4U);
  ck_assert_uint_eq(recv[0], 0x10);
  ck_assert_uint_eq(recv[3], 0x40);

  ck_assert_uint_eq((unsigned int)b.available(), 0U);
}
END_TEST

START_TEST(test_datagram_loopback_roundtrip)
{
  auto pair = DatagramPort::loopback();
  DatagramPort& a = pair.first;
  DatagramPort& b = pair.second;

  exec::WaitResult wait_before = b.wait_readable(3);
  ck_assert_msg(!wait_before.ready, "expected no datagram before send");

  Bytes packet = {0xAB, 0xCD};
  auto sendR = a.send(packet);
  ck_assert_msg(sendR.ready, "send should mark readable");

  exec::WaitResult wait_after = b.wait_readable(4);
  ck_assert_msg(wait_after.ready, "expected datagram after send");

  auto recv = b.recv();
  ck_assert_msg(recv.has_value(), "expected datagram payload");
  ck_assert_uint_eq((unsigned int)recv->size(), 2U);
  ck_assert_uint_eq((*recv)[0], 0xAB);
  ck_assert_uint_eq((*recv)[1], 0xCD);

  auto empty = b.recv();
  ck_assert_msg(!empty.has_value(), "expected empty inbox");
}
END_TEST

Suite* comms_primitives_suite(void) {
  Suite* s = suite_create("CommsPrimitives");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_channel_loopback_roundtrip);
  tcase_add_test(tc, test_datagram_loopback_roundtrip);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = comms_primitives_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
