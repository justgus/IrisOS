extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/parser.h"
#include "parser/tokenizer.h"

using namespace iris::parser;

namespace {

template <typename T>
unsigned int as_uint(T value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_tokenizer_basic)
{
  Tokenizer tokenizer("=,");
  auto result = tokenizer.tokenize("alpha = 42\nbeta=\"ok\"");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);

  ck_assert_int_eq((int)result.tokens[0].kind, (int)TokenKind::Identifier);
  ck_assert_str_eq(result.tokens[0].text.c_str(), "alpha");
  ck_assert_uint_eq(as_uint(result.tokens[0].span.line), 1U);
  ck_assert_uint_eq(as_uint(result.tokens[0].span.column), 1U);

  ck_assert_int_eq((int)result.tokens[1].kind, (int)TokenKind::Symbol);
  ck_assert_str_eq(result.tokens[1].text.c_str(), "=");

  ck_assert_int_eq((int)result.tokens[2].kind, (int)TokenKind::Number);
  ck_assert_str_eq(result.tokens[2].text.c_str(), "42");

  ck_assert_int_eq((int)result.tokens[3].kind, (int)TokenKind::Identifier);
  ck_assert_str_eq(result.tokens[3].text.c_str(), "beta");
  ck_assert_uint_eq(as_uint(result.tokens[3].span.line), 2U);
  ck_assert_uint_eq(as_uint(result.tokens[3].span.column), 1U);

  ck_assert_int_eq((int)result.tokens[4].kind, (int)TokenKind::Symbol);
  ck_assert_str_eq(result.tokens[4].text.c_str(), "=");

  ck_assert_int_eq((int)result.tokens[5].kind, (int)TokenKind::String);
  ck_assert_str_eq(result.tokens[5].text.c_str(), "ok");
}
END_TEST

START_TEST(test_tokenizer_error)
{
  Tokenizer tokenizer;
  auto result = tokenizer.tokenize("name=\"unterminated\n");
  ck_assert_uint_eq(as_uint(result.errors.size()), 1U);
  ck_assert_str_eq(result.errors[0].message.c_str(), "unterminated string");
  ck_assert_uint_eq(as_uint(result.errors[0].line), 1U);
}
END_TEST

START_TEST(test_parser_expect)
{
  Tokenizer tokenizer("=");
  auto result = tokenizer.tokenize("alpha");
  ParserCursor cursor(result.tokens);

  auto id = cursor.expect(TokenKind::Identifier, "expected identifier");
  ck_assert_msg(id.has_value(), "expected identifier token");

  auto sym = cursor.expect_symbol("=", "expected '='");
  ck_assert_msg(!sym.has_value(), "expected missing symbol");
  ck_assert_uint_eq(as_uint(cursor.errors().size()), 1U);
}
END_TEST

Suite* parser_core_suite(void) {
  Suite* s = suite_create("ParserCore");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_tokenizer_basic);
  tcase_add_test(tc, test_tokenizer_error);
  tcase_add_test(tc, test_parser_expect);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = parser_core_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
