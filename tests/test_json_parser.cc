extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/json_parser.h"

using namespace iris::parser;

namespace {

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_json_object)
{
  auto result = parse_json("{\"name\":\"alpha\",\"count\":3}");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_msg(result.value.has_value(), "expected value");

  const auto& value = result.value->value;
  ck_assert_msg(std::holds_alternative<ValueObject>(value), "expected object");
  const auto& obj = std::get<ValueObject>(value);
  ck_assert_uint_eq(as_uint(obj.size()), 2U);
  ck_assert_msg(std::holds_alternative<std::string>(obj.at("name").value), "expected name string");
  ck_assert_str_eq(std::get<std::string>(obj.at("name").value).c_str(), "alpha");
}
END_TEST

START_TEST(test_json_array)
{
  auto result = parse_json("[true, false, null, 1.5]");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_msg(result.value.has_value(), "expected value");
  ck_assert_msg(std::holds_alternative<ValueArray>(result.value->value), "expected array");
  const auto& arr = std::get<ValueArray>(result.value->value);
  ck_assert_uint_eq(as_uint(arr.size()), 4U);
}
END_TEST

START_TEST(test_json_string_escapes)
{
  auto result = parse_json("\"hello\\nworld\"");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_msg(result.value.has_value(), "expected value");
  ck_assert_msg(std::holds_alternative<std::string>(result.value->value), "expected string");
  ck_assert_str_eq(std::get<std::string>(result.value->value).c_str(), "hello\nworld");
}
END_TEST

START_TEST(test_json_error)
{
  auto result = parse_json("{\"a\":1,}");
  ck_assert_msg(!result.errors.empty(), "expected parse error");
}
END_TEST

Suite* json_parser_suite(void) {
  Suite* s = suite_create("JsonParser");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_json_object);
  tcase_add_test(tc, test_json_array);
  tcase_add_test(tc, test_json_string_escapes);
  tcase_add_test(tc, test_json_error);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = json_parser_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
