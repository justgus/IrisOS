extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/cpp_parser.h"

using namespace iris::parser;

namespace {

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_cpp_variable)
{
  auto result = parse_cpp("int count = 42;\nfloat rate;");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_uint_eq(as_uint(result.decls.size()), 2U);

  const auto& decl = result.decls[0];
  ck_assert_str_eq(decl.type.c_str(), "int");
  ck_assert_str_eq(decl.name.c_str(), "count");
  ck_assert_msg(decl.initializer.has_value(), "expected initializer");
  ck_assert_str_eq(decl.initializer->c_str(), "42");
}
END_TEST

START_TEST(test_cpp_function)
{
  auto result = parse_cpp("void reset(int value, float rate);");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_uint_eq(as_uint(result.decls.size()), 1U);

  const auto& decl = result.decls[0];
  ck_assert_int_eq(static_cast<int>(decl.kind), static_cast<int>(CppDecl::Kind::Function));
  ck_assert_str_eq(decl.name.c_str(), "reset");
  ck_assert_uint_eq(as_uint(decl.params.size()), 2U);
  ck_assert_str_eq(decl.params[0].name.c_str(), "value");
}
END_TEST

START_TEST(test_cpp_error)
{
  auto result = parse_cpp("int = 5;");
  ck_assert_msg(!result.errors.empty(), "expected parse error");
}
END_TEST

Suite* cpp_parser_suite(void) {
  Suite* s = suite_create("CppParser");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_cpp_variable);
  tcase_add_test(tc, test_cpp_function);
  tcase_add_test(tc, test_cpp_error);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = cpp_parser_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
