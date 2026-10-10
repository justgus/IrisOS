extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/conch_command.h"
#include "parser/conch_grammar.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace iris::parser;

namespace {

struct Fixture {
  std::string status;
  std::string input;
  std::string name;
  std::string kind;
  std::vector<std::string> args;
  std::string structured;
  std::string error;
  std::size_t error_offset{0};
  std::size_t error_line{0};
  std::size_t error_column{0};
};

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

std::vector<std::string> split_tab(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  std::stringstream ss(line);
  while (std::getline(ss, field, '\t')) fields.push_back(field);
  if (!line.empty() && line.back() == '\t') fields.emplace_back();
  return fields;
}

std::vector<std::string> split_args(const std::string& raw) {
  std::vector<std::string> args;
  if (raw.empty()) return args;

  std::string item;
  std::stringstream ss(raw);
  while (std::getline(ss, item, '|')) args.push_back(item);
  return args;
}

std::vector<Fixture> load_fixtures() {
  std::ifstream in(std::string(IRIS_TOP_SRCDIR) + "/tests/fixtures/parser/conch_commands.tsv");
  ck_assert_msg(in.good(), "failed to open parser fixture");

  std::vector<Fixture> fixtures;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto fields = split_tab(line);
    ck_assert_msg(fields.size() == 10, "invalid fixture field count");
    for (auto& field : fields) {
      if (field == "-") field.clear();
    }

    Fixture fixture;
    fixture.status = fields[0];
    fixture.input = fields[1];
    fixture.name = fields[2];
    fixture.kind = fields[3];
    fixture.args = split_args(fields[4]);
    fixture.structured = fields[5];
    fixture.error = fields[6];
    if (!fields[7].empty()) fixture.error_offset = std::stoull(fields[7]);
    if (!fields[8].empty()) fixture.error_line = std::stoull(fields[8]);
    if (!fields[9].empty()) fixture.error_column = std::stoull(fields[9]);
    fixtures.push_back(std::move(fixture));
  }
  return fixtures;
}

std::vector<std::string> split_semicolon(const std::string& raw) {
  std::vector<std::string> values;
  std::stringstream stream(raw);
  std::string value;
  while (std::getline(stream, value, ';')) values.push_back(value);
  return values;
}

void assert_payload_value(const ValueObject& payload, const std::string& expected) {
  const auto first = expected.find(':');
  const auto second = first == std::string::npos ? std::string::npos
                                                  : expected.find(':', first + 1);
  ck_assert_msg(first != std::string::npos && second != std::string::npos,
                "invalid structured payload fixture field: %s", expected.c_str());
  const auto name = expected.substr(0, first);
  const auto kind = expected.substr(first + 1, second - first - 1);
  const auto expected_value = expected.substr(second + 1);
  auto found = payload.find(name);
  ck_assert_msg(found != payload.end(), "missing structured payload field: %s", name.c_str());
  if (kind == "string") {
    ck_assert_msg(std::holds_alternative<std::string>(found->second.value),
                  "expected string field %s", name.c_str());
    ck_assert_str_eq(std::get<std::string>(found->second.value).c_str(), expected_value.c_str());
  } else if (kind == "bool") {
    ck_assert_msg(std::holds_alternative<bool>(found->second.value),
                  "expected boolean field %s", name.c_str());
    ck_assert_int_eq(std::get<bool>(found->second.value), expected_value == "true");
  } else if (kind == "int64") {
    ck_assert_msg(std::holds_alternative<std::int64_t>(found->second.value),
                  "expected signed integer field %s", name.c_str());
    ck_assert_int_eq(std::get<std::int64_t>(found->second.value), std::stoll(expected_value));
  } else if (kind == "uint64") {
    ck_assert_msg(std::holds_alternative<std::uint64_t>(found->second.value),
                  "expected unsigned integer field %s", name.c_str());
    ck_assert_uint_eq(std::get<std::uint64_t>(found->second.value), std::stoull(expected_value));
  } else {
    ck_abort_msg("unknown structured payload fixture kind: %s", kind.c_str());
  }
}

void assert_structured(const Fixture& fixture, const CommandAst& ast) {
  if (fixture.structured.empty()) return;
  auto fields = split_semicolon(fixture.structured);
  ck_assert_msg(fields.size() >= 2, "invalid structured fixture: %s", fixture.structured.c_str());
  if (fields[0] == "task") {
    auto command = ast.get_if<TaskCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_msg(fields[1] == "Start" && command->kind == TaskCommandKind::Start,
                  "expected start task command");
    ck_assert_uint_eq(as_uint(command->args.size()), as_uint(fields.size() - 2));
    for (std::size_t i = 2; i < fields.size(); ++i) {
      ck_assert_str_eq(command->args[i - 2].c_str(), fields[i].c_str());
    }
  } else if (fields[0] == "schema_show") {
    auto command = ast.get_if<SchemaCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_msg(fields[1] == "ShowType" && command->kind == SchemaCommandKind::ShowType,
                  "expected show type schema command");
    ck_assert_str_eq(command->type_name.c_str(), fields[2].c_str());
  } else if (fields[0] == "object_show") {
    auto command = ast.get_if<ObjectCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_msg(fields[1] == "Show" && command->kind == ObjectCommandKind::Show,
                  "expected show object command");
    ck_assert_str_eq(command->target.c_str(), fields[2].c_str());
  } else if (fields[0] == "alias") {
    auto command = ast.get_if<AliasAssignmentCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_str_eq(command->keyword.c_str(), fields[1].c_str());
    ck_assert_str_eq(command->name.c_str(), fields[2].c_str());
    ck_assert_int_eq(command->persistent, fields[3] == "true");
    ck_assert_ptr_nonnull(command->authoring ? &*command->authoring : nullptr);
    ck_assert_msg(command->expression.empty(), "authoring alias should not retain raw expression");
    ck_assert_str_eq(command->authoring->type_name.c_str(), fields[4].c_str());
    const auto& payload = std::get<ValueObject>(command->authoring->payload.value);
    for (std::size_t i = 5; i < fields.size(); ++i) assert_payload_value(payload, fields[i]);
  } else if (fields[0] == "new" || fields[0] == "jsonnew") {
    auto command = ast.get_if<ObjectCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_ptr_nonnull(command->authoring ? &*command->authoring : nullptr);
    ck_assert_str_eq(command->authoring->type_name.c_str(), fields[1].c_str());
    auto payload = std::get_if<ValueObject>(&command->authoring->payload.value);
    ck_assert_ptr_nonnull(payload);
    for (std::size_t i = 2; i < fields.size(); ++i) assert_payload_value(*payload, fields[i]);
    if (fields[0] == "jsonnew") {
      ck_assert_msg(command->authoring->json_mode, "expected JSON new fixture");
    }
  } else if (fields[0] == "define") {
    auto command = ast.get_if<SchemaCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_msg(command->kind == SchemaCommandKind::DefineType,
                  "expected define type schema command");
    ck_assert_ptr_nonnull(command->authoring ? &*command->authoring : nullptr);
    ck_assert_msg(!command->authoring->json_mode, "expected inline define fixture");
    ck_assert_str_eq(command->authoring->type_name.c_str(), fields[1].c_str());
    ck_assert_uint_eq(as_uint(command->authoring->fields.size()), as_uint(fields.size() - 2));
    for (std::size_t i = 2; i < fields.size(); ++i) {
      const auto first = fields[i].find(':');
      const auto second = first == std::string::npos ? std::string::npos
                                                      : fields[i].find(':', first + 1);
      ck_assert_msg(first != std::string::npos && second != std::string::npos,
                    "invalid field specification fixture: %s", fields[i].c_str());
      const auto& actual = command->authoring->fields[i - 2];
      ck_assert_str_eq(actual.name.c_str(), fields[i].substr(0, first).c_str());
      ck_assert_str_eq(actual.type_name.c_str(),
                       fields[i].substr(first + 1, second - first - 1).c_str());
      ck_assert_int_eq(actual.required, fields[i].substr(second + 1) == "required");
    }
  } else if (fields[0] == "jsondefine") {
    auto command = ast.get_if<SchemaCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_msg(command->kind == SchemaCommandKind::DefineType,
                  "expected JSON define type schema command");
    ck_assert_ptr_nonnull(command->authoring ? &*command->authoring : nullptr);
    ck_assert_str_eq(command->authoring->type_name.c_str(), fields[1].c_str());
    ck_assert_msg(command->authoring->json_mode, "expected JSON define fixture");
    const auto& payload = std::get<ValueObject>(command->authoring->payload.value);
    for (std::size_t i = 2; i < fields.size(); ++i) assert_payload_value(payload, fields[i]);
  } else if (fields[0] == "alias_plain") {
    auto command = ast.get_if<AliasAssignmentCommand>();
    ck_assert_ptr_nonnull(command);
    ck_assert_str_eq(command->keyword.c_str(), fields[1].c_str());
    ck_assert_str_eq(command->name.c_str(), fields[2].c_str());
    ck_assert_int_eq(command->persistent, fields[3] == "true");
    ck_assert_str_eq(command->expression.c_str(), fields[4].c_str());
  }
}

void assert_normalized(const Fixture& fixture, const CommandAst& ast) {
  auto normalized = normalize_command(ast);
  ck_assert_str_eq(normalized.name.c_str(), fixture.name.c_str());
  ck_assert_str_eq(normalized.node_kind.c_str(), fixture.kind.c_str());
  ck_assert_msg(normalized.args.size() == fixture.args.size(),
                "argument count mismatch for %s: got %zu expected %zu",
                fixture.input.c_str(), normalized.args.size(), fixture.args.size());
  for (std::size_t i = 0; i < fixture.args.size(); ++i) {
    ck_assert_str_eq(normalized.args[i].c_str(), fixture.args[i].c_str());
  }
}

} // namespace

START_TEST(test_shared_parser_fixtures_match_shell_and_reusable_api)
{
  auto fixtures = load_fixtures();
  ck_assert_msg(!fixtures.empty(), "expected fixtures");

  for (const auto& fixture : fixtures) {
    auto shell_ast = parse_conch_command(fixture.input);
    auto reusable = parse_conch_grammar(fixture.input);

    ck_assert_uint_eq(as_uint(shell_ast.errors.size()), as_uint(reusable.ast.errors.size()));
    if (fixture.status == "ok") {
      ck_assert_msg(shell_ast.errors.empty(), "unexpected shell parse error for %s",
                    fixture.input.c_str());
      ck_assert_msg(reusable.ast.errors.empty(), "unexpected reusable parse error for %s",
                    fixture.input.c_str());
      assert_normalized(fixture, shell_ast);
      assert_normalized(fixture, reusable.ast);
      assert_structured(fixture, shell_ast);
      assert_structured(fixture, reusable.ast);
    } else {
      ck_assert_msg(!shell_ast.errors.empty(), "expected shell parse error for %s",
                    fixture.input.c_str());
      ck_assert_msg(!reusable.ast.errors.empty(), "expected reusable parse error for %s",
                    fixture.input.c_str());
      ck_assert_str_eq(shell_ast.errors[0].message.c_str(), fixture.error.c_str());
      ck_assert_str_eq(reusable.ast.errors[0].message.c_str(), fixture.error.c_str());
      ck_assert_str_eq(shell_ast.errors[0].message.c_str(),
                       reusable.ast.errors[0].message.c_str());
      ck_assert_uint_eq(as_uint(shell_ast.errors[0].offset), as_uint(fixture.error_offset));
      ck_assert_uint_eq(as_uint(shell_ast.errors[0].line), as_uint(fixture.error_line));
      ck_assert_uint_eq(as_uint(shell_ast.errors[0].column), as_uint(fixture.error_column));
      ck_assert_uint_eq(as_uint(reusable.ast.errors[0].offset), as_uint(fixture.error_offset));
      ck_assert_uint_eq(as_uint(reusable.ast.errors[0].line), as_uint(fixture.error_line));
      ck_assert_uint_eq(as_uint(reusable.ast.errors[0].column), as_uint(fixture.error_column));
    }
  }
}
END_TEST

Suite* conch_parser_regression_suite(void) {
  Suite* s = suite_create("ConchParserRegression");
  TCase* tc = tcase_create("fixtures");

  tcase_add_test(tc, test_shared_parser_fixtures_match_shell_and_reusable_api);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = conch_parser_regression_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
