extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/conch_batch.h"
#include "parser/conch_command.h"
#include "parser/conch_grammar.h"

using namespace iris::parser;

namespace {

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_reusable_parser_returns_typed_ast)
{
  auto result = parse_conch_grammar("call demo expand 2");
  ck_assert_uint_eq(as_uint(result.ast.errors.size()), 0U);
  auto command = result.ast.get_if<CallCommand>();
  ck_assert_ptr_nonnull(command);
  ck_assert_str_eq(command->target.c_str(), "demo");
  ck_assert_str_eq(command->operation.c_str(), "expand");

  auto normalized = normalize_command(result.ast);
  ck_assert_str_eq(normalized.name.c_str(), "call");
  ck_assert_str_eq(normalized.node_kind.c_str(), "Call");
  ck_assert_uint_eq(as_uint(normalized.args.size()), 3U);
}
END_TEST

START_TEST(test_reusable_parser_preserves_parse_errors)
{
  auto shell_ast = parse_conch_command("say \"oops\n");
  auto reusable = parse_conch_grammar("say \"oops\n");

  ck_assert_uint_eq(as_uint(shell_ast.errors.size()), 1U);
  ck_assert_uint_eq(as_uint(reusable.ast.errors.size()), 1U);
  ck_assert_str_eq(reusable.ast.errors[0].message.c_str(), shell_ast.errors[0].message.c_str());
  ck_assert_uint_eq(as_uint(reusable.ast.errors[0].line), as_uint(shell_ast.errors[0].line));
  ck_assert_uint_eq(as_uint(reusable.ast.errors[0].column), as_uint(shell_ast.errors[0].column));
}
END_TEST

START_TEST(test_batch_executes_supported_commands_in_order)
{
  auto result = execute_conch_batch(
      "namespace Demo\n"
      "show type Demo::Widget\n"
      "call demo expand 2\n");

  ck_assert_msg(result.ok, "expected batch success");
  ck_assert_msg(!result.failure.has_value(), "did not expect failure");
  ck_assert_uint_eq(as_uint(result.completed.size()), 3U);
  ck_assert_str_eq(result.completed[0].command.node_kind.c_str(), "Namespace");
  ck_assert_str_eq(result.completed[1].command.node_kind.c_str(), "Schema");
  ck_assert_str_eq(result.completed[2].command.node_kind.c_str(), "Call");
  ck_assert_uint_eq(as_uint(result.completed[2].line), 3U);
}
END_TEST

START_TEST(test_batch_executor_callback_runs_commands_in_order)
{
  std::vector<std::string> executed;
  auto result = execute_conch_batch(
      "namespace Demo\n"
      "show type Demo::Widget\n"
      "call demo expand 2\n",
      [&](const CommandAst& ast) -> std::optional<std::string> {
        executed.push_back(ast.name);
        return std::nullopt;
      });

  ck_assert_msg(result.ok, "expected batch success");
  ck_assert_uint_eq(as_uint(result.completed.size()), 3U);
  ck_assert_uint_eq(as_uint(executed.size()), 3U);
  ck_assert_str_eq(executed[0].c_str(), "namespace");
  ck_assert_str_eq(executed[1].c_str(), "show");
  ck_assert_str_eq(executed[2].c_str(), "call");
}
END_TEST

START_TEST(test_batch_stops_on_first_parse_error_with_partial_results)
{
  auto result = execute_conch_batch(
      "namespace Demo\n"
      "say \"oops\n"
      "show type Demo::Widget\n");

  ck_assert_msg(!result.ok, "expected batch failure");
  ck_assert_uint_eq(as_uint(result.completed.size()), 1U);
  ck_assert_msg(result.failure.has_value(), "expected failure detail");
  ck_assert_int_eq(static_cast<int>(result.failure->status),
                   static_cast<int>(BatchCommandStatus::ParseError));
  ck_assert_uint_eq(as_uint(result.failure->command_index), 1U);
  ck_assert_uint_eq(as_uint(result.failure->line), 2U);
  ck_assert_str_eq(result.failure->message.c_str(), "unterminated string");
}
END_TEST

START_TEST(test_batch_stops_on_first_execution_error_with_partial_results)
{
  std::vector<std::string> executed;
  auto result = execute_conch_batch(
      "namespace Demo\n"
      "show type Demo::Widget\n"
      "call demo expand 2\n",
      [&](const CommandAst& ast) -> std::optional<std::string> {
        executed.push_back(ast.name);
        if (ast.name == "show") return std::string("execution failed");
        return std::nullopt;
      });

  ck_assert_msg(!result.ok, "expected batch failure");
  ck_assert_uint_eq(as_uint(result.completed.size()), 1U);
  ck_assert_uint_eq(as_uint(executed.size()), 2U);
  ck_assert_msg(result.failure.has_value(), "expected failure detail");
  ck_assert_int_eq(static_cast<int>(result.failure->status),
                   static_cast<int>(BatchCommandStatus::ExecutionError));
  ck_assert_uint_eq(as_uint(result.failure->command_index), 1U);
  ck_assert_uint_eq(as_uint(result.failure->line), 2U);
  ck_assert_str_eq(result.failure->message.c_str(), "execution failed");
}
END_TEST

START_TEST(test_batch_stops_on_first_unsupported_command_with_partial_results)
{
  auto result = execute_conch_batch(
      "namespace Demo\n"
      "help\n"
      "show type Demo::Widget\n");

  ck_assert_msg(!result.ok, "expected batch failure");
  ck_assert_uint_eq(as_uint(result.completed.size()), 1U);
  ck_assert_msg(result.failure.has_value(), "expected failure detail");
  ck_assert_int_eq(static_cast<int>(result.failure->status),
                   static_cast<int>(BatchCommandStatus::Unsupported));
  ck_assert_uint_eq(as_uint(result.failure->command_index), 1U);
  ck_assert_str_eq(result.failure->message.c_str(), "unsupported command");
}
END_TEST

Suite* conch_grammar_api_suite(void) {
  Suite* s = suite_create("ConchGrammarApi");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_reusable_parser_returns_typed_ast);
  tcase_add_test(tc, test_reusable_parser_preserves_parse_errors);
  tcase_add_test(tc, test_batch_executes_supported_commands_in_order);
  tcase_add_test(tc, test_batch_executor_callback_runs_commands_in_order);
  tcase_add_test(tc, test_batch_stops_on_first_parse_error_with_partial_results);
  tcase_add_test(tc, test_batch_stops_on_first_execution_error_with_partial_results);
  tcase_add_test(tc, test_batch_stops_on_first_unsupported_command_with_partial_results);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = conch_grammar_api_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
