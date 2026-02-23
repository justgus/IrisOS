extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/conch_command.h"

using namespace iris::parser;

namespace {

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_conch_parser_quotes)
{
  auto ast = parse_conch_command("emit viz textlog \"hello world\" --role artifact");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  ck_assert_str_eq(ast.name.c_str(), "emit");
  ck_assert_uint_eq(as_uint(ast.args.size()), 5U);
  ck_assert_str_eq(ast.args[0].c_str(), "viz");
  ck_assert_str_eq(ast.args[1].c_str(), "textlog");
  ck_assert_str_eq(ast.args[2].c_str(), "hello world");
  ck_assert_str_eq(ast.args[3].c_str(), "--role");
  ck_assert_str_eq(ast.args[4].c_str(), "artifact");
}
END_TEST

START_TEST(test_conch_parser_unterminated)
{
  auto ast = parse_conch_command("say \"oops\n");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 1U);
  ck_assert_str_eq(ast.errors[0].message.c_str(), "unterminated string");
}
END_TEST

Suite* conch_parser_suite(void) {
  Suite* s = suite_create("ConchParser");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_conch_parser_quotes);
  tcase_add_test(tc, test_conch_parser_unterminated);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = conch_parser_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
