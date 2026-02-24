extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/python_parser.h"

using namespace iris::parser;

namespace {

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_python_function)
{
  const char* input = "def greet():\n    pass\nname = \"ok\"\n";
  auto result = parse_python(input);
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_uint_eq(as_uint(result.statements.size()), 2U);
  ck_assert_int_eq(static_cast<int>(result.statements[0].kind),
                   static_cast<int>(PyStmt::Kind::FunctionDef));
  ck_assert_str_eq(result.statements[0].name.c_str(), "greet");
  ck_assert_uint_eq(as_uint(result.statements[0].body.size()), 1U);
  ck_assert_int_eq(static_cast<int>(result.statements[1].kind),
                   static_cast<int>(PyStmt::Kind::Assign));
}
END_TEST

START_TEST(test_python_assign)
{
  auto result = parse_python("value = 3\n");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_uint_eq(as_uint(result.statements.size()), 1U);
  ck_assert_str_eq(result.statements[0].name.c_str(), "value");
  ck_assert_msg(result.statements[0].value.has_value(), "expected value");
}
END_TEST

START_TEST(test_python_indent_error)
{
  auto result = parse_python("def bad():\n  pass\n   pass\n");
  ck_assert_msg(!result.errors.empty(), "expected indentation error");
}
END_TEST

Suite* python_parser_suite(void) {
  Suite* s = suite_create("PythonParser");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_python_function);
  tcase_add_test(tc, test_python_assign);
  tcase_add_test(tc, test_python_indent_error);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = python_parser_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
