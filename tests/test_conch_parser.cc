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

START_TEST(test_conch_parser_alias_assignment_typed)
{
  auto ast = parse_conch_command("let demo=new Demo::Widget label:=\"hello world\"");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  auto command = ast.get_if<AliasAssignmentCommand>();
  ck_assert_ptr_nonnull(command);
  ck_assert_msg(!command->persistent, "expected session alias");
  ck_assert_msg(!command->list_aliases, "expected assignment");
  ck_assert_str_eq(command->name.c_str(), "demo");
  ck_assert_str_eq(command->expression.c_str(), "new Demo::Widget label:=\"hello world\"");
}
END_TEST

START_TEST(test_conch_parser_schema_command_typed)
{
  auto ast = parse_conch_command("show type Demo::Widget");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  auto command = ast.get_if<SchemaCommand>();
  ck_assert_ptr_nonnull(command);
  ck_assert_int_eq(static_cast<int>(command->kind),
                   static_cast<int>(SchemaCommandKind::ShowType));
  ck_assert_str_eq(command->type_name.c_str(), "Demo::Widget");
}
END_TEST

START_TEST(test_conch_parser_object_command_typed)
{
  auto ast = parse_conch_command("new Demo::Widget label:=alpha");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  auto command = ast.get_if<ObjectCommand>();
  ck_assert_ptr_nonnull(command);
  ck_assert_int_eq(static_cast<int>(command->kind),
                   static_cast<int>(ObjectCommandKind::New));
  ck_assert_str_eq(command->expression.c_str(), "new Demo::Widget label:=alpha");
}
END_TEST

START_TEST(test_conch_parser_call_command_typed)
{
  auto ast = parse_conch_command("call demo expand 2");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  auto command = ast.get_if<CallCommand>();
  ck_assert_ptr_nonnull(command);
  ck_assert_str_eq(command->target.c_str(), "demo");
  ck_assert_str_eq(command->operation.c_str(), "expand");
  ck_assert_uint_eq(as_uint(command->args.size()), 1U);
  ck_assert_str_eq(command->args[0].c_str(), "2");
}
END_TEST

START_TEST(test_conch_parser_task_and_io_commands_typed)
{
  auto task_ast = parse_conch_command("task spawn demo service");
  ck_assert_uint_eq(as_uint(task_ast.errors.size()), 0U);
  auto task = task_ast.get_if<TaskCommand>();
  ck_assert_ptr_nonnull(task);
  ck_assert_int_eq(static_cast<int>(task->kind), static_cast<int>(TaskCommandKind::Task));
  ck_assert_uint_eq(as_uint(task->args.size()), 3U);

  auto io_ast = parse_conch_command("io send tx 0a0b");
  ck_assert_uint_eq(as_uint(io_ast.errors.size()), 0U);
  auto io = io_ast.get_if<IoCommand>();
  ck_assert_ptr_nonnull(io);
  ck_assert_uint_eq(as_uint(io->args.size()), 3U);
  ck_assert_str_eq(io->args[0].c_str(), "send");
}
END_TEST

START_TEST(test_conch_parser_namespace_commands_typed)
{
  auto ns_ast = parse_conch_command("namespace NavTest::Inner");
  ck_assert_uint_eq(as_uint(ns_ast.errors.size()), 0U);
  auto ns = ns_ast.get_if<NamespaceCommand>();
  ck_assert_ptr_nonnull(ns);
  ck_assert_str_eq(ns->keyword.c_str(), "namespace");
  ck_assert_uint_eq(as_uint(ns->args.size()), 1U);
  ck_assert_str_eq(ns->args[0].c_str(), "NavTest::Inner");

  auto ls_ast = parse_conch_command("ls --recursive --objects");
  ck_assert_uint_eq(as_uint(ls_ast.errors.size()), 0U);
  auto list = ls_ast.get_if<TypesListCommand>();
  ck_assert_ptr_nonnull(list);
  ck_assert_uint_eq(as_uint(list->args.size()), 2U);
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
  tcase_add_test(tc, test_conch_parser_alias_assignment_typed);
  tcase_add_test(tc, test_conch_parser_schema_command_typed);
  tcase_add_test(tc, test_conch_parser_object_command_typed);
  tcase_add_test(tc, test_conch_parser_call_command_typed);
  tcase_add_test(tc, test_conch_parser_task_and_io_commands_typed);
  tcase_add_test(tc, test_conch_parser_namespace_commands_typed);
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
