extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/conch_command.h"
#include "parser/json_parser.h"

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
  ck_assert_ptr_nonnull(command->authoring ? &*command->authoring : nullptr);
  ck_assert_str_eq(command->authoring->type_name.c_str(), "Demo::Widget");
  ck_assert_str_eq(command->expression.c_str(), "");
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
  ck_assert_ptr_nonnull(command->authoring ? &*command->authoring : nullptr);
  ck_assert_str_eq(command->authoring->type_name.c_str(), "Demo::Widget");
  const auto& payload = std::get<ValueObject>(command->authoring->payload.value);
  ck_assert_str_eq(std::get<std::string>(payload.at("label").value).c_str(), "alpha");
}
END_TEST

START_TEST(test_conch_parser_authoring_values_are_structured_and_lossless)
{
  auto ast = parse_conch_command(
      "new Demo::Widget low:=-9223372036854775808 high:=9223372036854775807 "
      "single:='it\\'s fine' double:=\"escaped \\\"quote\\\"\" "
      "slash:=\"back\\\\slash\" compat:=\"\\q \\t C:\\temp\"");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  auto command = ast.get_if<ObjectCommand>();
  ck_assert_ptr_nonnull(command);
  const auto& fields = std::get<ValueObject>(command->authoring->payload.value);
  ck_assert_int_eq(std::get<std::int64_t>(fields.at("low").value), INT64_MIN);
  ck_assert_int_eq(std::get<std::int64_t>(fields.at("high").value), INT64_MAX);
  ck_assert_str_eq(std::get<std::string>(fields.at("single").value).c_str(), "it's fine");
  ck_assert_str_eq(std::get<std::string>(fields.at("double").value).c_str(), "escaped \"quote\"");
  ck_assert_str_eq(std::get<std::string>(fields.at("slash").value).c_str(), "back\\slash");
  ck_assert_str_eq(std::get<std::string>(fields.at("compat").value).c_str(), "q t C:temp");

  const std::string json_input =
      "new --json '{\"type\":\"Demo::Widget\",\"payload\":{\"n\":9007199254740993}}'";
  auto json = parse_conch_command(json_input);
  ck_assert_uint_eq(as_uint(json.errors.size()), 0U);
  auto json_command = json.get_if<ObjectCommand>();
  ck_assert_ptr_nonnull(json_command);
  const auto& json_fields = std::get<ValueObject>(json_command->authoring->payload.value);
  ck_assert_uint_eq(std::get<std::uint64_t>(json_fields.at("n").value),
                    UINT64_C(9007199254740993));
  ck_assert_uint_eq(as_uint(json_fields.at("n").span.offset),
                    as_uint(json_input.find("9007199254740993")));
  ck_assert_uint_eq(as_uint(json_fields.at("n").span.column),
                    as_uint(json_input.find("9007199254740993") + 1));

  auto floating_json = parse_conch_command(
      "new\t--json '{\"type\":\"Demo::Widget\",\"payload\":{\"n\":1.25}}'");
  ck_assert_uint_eq(as_uint(floating_json.errors.size()), 0U);
  const auto& floating_fields = std::get<ValueObject>(
      floating_json.get_if<ObjectCommand>()->authoring->payload.value);
  ck_assert_double_eq(std::get<double>(floating_fields.at("n").value), 1.25);

  auto malformed_json = parse_conch_command("new --json '{\"type\":,}'");
  ck_assert_uint_eq(as_uint(malformed_json.errors.size()), 1U);
  ck_assert_msg(!malformed_json.get_if<ObjectCommand>(), "malformed JSON must not create an AST");

  auto alias = parse_conch_command("let item=new Demo::Widget n:=9007199254740993");
  ck_assert_uint_eq(as_uint(alias.errors.size()), 0U);
  auto alias_command = alias.get_if<AliasAssignmentCommand>();
  ck_assert_ptr_nonnull(alias_command);
  ck_assert_ptr_nonnull(alias_command->authoring ? &*alias_command->authoring : nullptr);
  const auto& alias_fields = std::get<ValueObject>(alias_command->authoring->payload.value);
  ck_assert_int_eq(std::get<std::int64_t>(alias_fields.at("n").value),
                   INT64_C(9007199254740993));
  auto direct = parse_conch_command("new Demo::Widget n:=9007199254740993");
  ck_assert_uint_eq(as_uint(direct.errors.size()), 0U);
  auto direct_fields = std::get<ValueObject>(
      direct.get_if<ObjectCommand>()->authoring->payload.value);
  ck_assert_int_eq(std::get<std::int64_t>(alias_fields.at("n").value),
                   std::get<std::int64_t>(direct_fields.at("n").value));
}
END_TEST

START_TEST(test_json_parser_keeps_integer_kinds)
{
  auto parsed = parse_json("[9007199254740993,18446744073709551615,-9223372036854775808,1.25]");
  ck_assert_uint_eq(as_uint(parsed.errors.size()), 0U);
  const auto& values = std::get<ValueArray>(parsed.value->value);
  ck_assert_uint_eq(std::get<std::uint64_t>(values[0].value), UINT64_C(9007199254740993));
  ck_assert_uint_eq(std::get<std::uint64_t>(values[1].value), UINT64_MAX);
  ck_assert_int_eq(std::get<std::int64_t>(values[2].value), INT64_MIN);
  ck_assert_double_eq(std::get<double>(values[3].value), 1.25);
}
END_TEST

START_TEST(test_conch_parser_define_type_has_structured_fields)
{
  auto ast = parse_conch_command("define type Demo::Record fields id:Integer, label?:String");
  ck_assert_uint_eq(as_uint(ast.errors.size()), 0U);
  auto schema = ast.get_if<SchemaCommand>();
  ck_assert_ptr_nonnull(schema);
  ck_assert_ptr_nonnull(schema->authoring ? &*schema->authoring : nullptr);
  ck_assert_str_eq(schema->authoring->type_name.c_str(), "Demo::Record");
  ck_assert_uint_eq(as_uint(schema->authoring->fields.size()), 2U);
  ck_assert_str_eq(schema->authoring->fields[0].name.c_str(), "id");
  ck_assert_msg(schema->authoring->fields[0].required, "id should be required");
  ck_assert_str_eq(schema->authoring->fields[1].name.c_str(), "label");
  ck_assert_msg(!schema->authoring->fields[1].required, "label should be optional");

  auto json = parse_conch_command(
      "define type --json '{\"name\":\"Record\",\"namespace\":\"Demo\","
      "\"version\":1,\"type_id\":9007199254740993}'");
  ck_assert_uint_eq(as_uint(json.errors.size()), 0U);
  auto json_schema = json.get_if<SchemaCommand>();
  ck_assert_ptr_nonnull(json_schema);
  ck_assert_msg(json_schema->authoring->json_mode, "expected explicit JSON mode");
  ck_assert_str_eq(json_schema->authoring->type_name.c_str(), "Demo::Record");
  ck_assert_str_eq(json_schema->type_name.c_str(), "Demo::Record");
  const auto& json_root = std::get<ValueObject>(json_schema->authoring->payload.value);
  ck_assert_uint_eq(std::get<std::uint64_t>(json_root.at("type_id").value),
                    UINT64_C(9007199254740993));

  auto json_float = parse_conch_command(
      "define\ttype --json '{\"name\":\"Record\",\"version\":1.25}'");
  ck_assert_uint_eq(as_uint(json_float.errors.size()), 0U);
  const auto& float_schema = std::get<ValueObject>(
      json_float.get_if<SchemaCommand>()->authoring->payload.value);
  ck_assert_double_eq(std::get<double>(float_schema.at("version").value), 1.25);

  auto malformed_json = parse_conch_command("define type --json '{\"name\":,}'");
  ck_assert_uint_eq(as_uint(malformed_json.errors.size()), 1U);
}
END_TEST

START_TEST(test_conch_parser_authoring_rejects_malformed_shapes)
{
  auto new_ast = parse_conch_command("new Demo::Widget label:=");
  ck_assert_uint_eq(as_uint(new_ast.errors.size()), 1U);
  ck_assert_str_eq(new_ast.errors[0].message.c_str(), "expected field:=value");
  ck_assert_uint_eq(as_uint(new_ast.errors[0].line), 1U);
  ck_assert_msg(new_ast.errors[0].column > 1U, "expected value error column");

  auto define_ast = parse_conch_command("define type Demo::Record fields id:");
  ck_assert_uint_eq(as_uint(define_ast.errors.size()), 1U);
  ck_assert_str_eq(define_ast.errors[0].message.c_str(), "invalid field specification");
  ck_assert_msg(define_ast.errors[0].column > 1U, "expected field error column");

  auto invalid_field = parse_conch_command("new Demo::Widget bad-name:=x");
  ck_assert_uint_eq(as_uint(invalid_field.errors.size()), 1U);
  ck_assert_str_eq(invalid_field.errors[0].message.c_str(), "invalid field name");

  auto spaced = parse_conch_command("new\tDemo::Widget\tvalue\t:=\ttext");
  ck_assert_uint_eq(as_uint(spaced.errors.size()), 0U);
  ck_assert_ptr_nonnull(spaced.get_if<ObjectCommand>());

  auto spaced_define = parse_conch_command("define\ttype Demo::Record fields id : Integer");
  ck_assert_uint_eq(as_uint(spaced_define.errors.size()), 0U);
  ck_assert_ptr_nonnull(spaced_define.get_if<SchemaCommand>());
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

START_TEST(test_conch_tokenizer_covers_conch_lexical_forms)
{
  ConchTokenizer tokenizer;
  auto result = tokenizer.tokenize(
      "Demo::Widget dotted.name -12 := = -- { } [ ] , : "
      "'it\\'s \"fine\"' \"say \\\"hi\\\"\" 'slash\\\\path' \"slash\\\\path\"");
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);

  std::vector<Token> tokens;
  for (const auto& token : result.tokens) {
    if (token.kind != TokenKind::End) tokens.push_back(token);
  }
  ck_assert_uint_eq(as_uint(tokens.size()), 16U);
  ck_assert_int_eq(static_cast<int>(tokens[0].kind), static_cast<int>(TokenKind::Identifier));
  ck_assert_str_eq(tokens[0].text.c_str(), "Demo::Widget");
  ck_assert_int_eq(static_cast<int>(tokens[1].kind), static_cast<int>(TokenKind::Identifier));
  ck_assert_str_eq(tokens[1].text.c_str(), "dotted.name");
  ck_assert_int_eq(static_cast<int>(tokens[2].kind), static_cast<int>(TokenKind::Number));
  ck_assert_str_eq(tokens[2].text.c_str(), "-12");
  ck_assert_str_eq(tokens[3].text.c_str(), ":=");
  ck_assert_str_eq(tokens[4].text.c_str(), "=");
  ck_assert_str_eq(tokens[5].text.c_str(), "--");
  ck_assert_str_eq(tokens[6].text.c_str(), "{");
  ck_assert_str_eq(tokens[7].text.c_str(), "}");
  ck_assert_str_eq(tokens[8].text.c_str(), "[");
  ck_assert_str_eq(tokens[9].text.c_str(), "]");
  ck_assert_str_eq(tokens[10].text.c_str(), ",");
  ck_assert_str_eq(tokens[11].text.c_str(), ":");
  ck_assert_int_eq(static_cast<int>(tokens[12].kind), static_cast<int>(TokenKind::String));
  ck_assert_str_eq(tokens[12].text.c_str(), "it's \"fine\"");
  ck_assert_str_eq(tokens[13].text.c_str(), "say \"hi\"");
  ck_assert_str_eq(tokens[14].text.c_str(), "slash\\path");
  ck_assert_str_eq(tokens[15].text.c_str(), "slash\\path");
}
END_TEST

START_TEST(test_conch_tokenizer_rejects_invalid_identifier_continuations)
{
  ConchTokenizer tokenizer;
  auto result = tokenizer.tokenize("9invalid valid-name");
  ck_assert_uint_eq(as_uint(result.errors.size()), 1U);
  bool found_invalid_identifier = false;
  for (const auto& token : result.tokens) {
    if (token.text == "9invalid" || token.text == "valid-name") {
      found_invalid_identifier = true;
    }
  }
  ck_assert_msg(!found_invalid_identifier,
                "invalid identifier prefixes and hyphens must not form one token");
}
END_TEST

START_TEST(test_conch_parser_caliper_commands_typed)
{
  auto list_ast = parse_conch_command("caliper list");
  ck_assert_uint_eq(as_uint(list_ast.errors.size()), 0U);
  auto list = list_ast.get_if<CaliperCommand>();
  ck_assert_ptr_nonnull(list);
  ck_assert_uint_eq(as_uint(list->args.size()), 1U);
  ck_assert_str_eq(list->args[0].c_str(), "list");

  auto convert_ast = parse_conch_command("caliper convert 32 °F °C --dimension");
  ck_assert_uint_eq(as_uint(convert_ast.errors.size()), 0U);
  auto convert = convert_ast.get_if<CaliperCommand>();
  ck_assert_ptr_nonnull(convert);
  ck_assert_uint_eq(as_uint(convert->args.size()), 5U);
  ck_assert_str_eq(convert->args[0].c_str(), "convert");
  ck_assert_str_eq(convert->args[4].c_str(), "--dimension");
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
  tcase_add_test(tc, test_conch_parser_authoring_values_are_structured_and_lossless);
  tcase_add_test(tc, test_json_parser_keeps_integer_kinds);
  tcase_add_test(tc, test_conch_parser_define_type_has_structured_fields);
  tcase_add_test(tc, test_conch_parser_authoring_rejects_malformed_shapes);
  tcase_add_test(tc, test_conch_parser_call_command_typed);
  tcase_add_test(tc, test_conch_parser_task_and_io_commands_typed);
  tcase_add_test(tc, test_conch_parser_namespace_commands_typed);
  tcase_add_test(tc, test_conch_tokenizer_covers_conch_lexical_forms);
  tcase_add_test(tc, test_conch_tokenizer_rejects_invalid_identifier_continuations);
  tcase_add_test(tc, test_conch_parser_caliper_commands_typed);
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
