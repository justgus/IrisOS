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
  std::string error;
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
    ck_assert_msg(fields.size() == 6, "invalid fixture field count");

    Fixture fixture;
    fixture.status = fields[0];
    fixture.input = fields[1];
    fixture.name = fields[2];
    fixture.kind = fields[3];
    fixture.args = split_args(fields[4]);
    fixture.error = fields[5];
    fixtures.push_back(std::move(fixture));
  }
  return fixtures;
}

void assert_normalized(const Fixture& fixture, const CommandAst& ast) {
  auto normalized = normalize_command(ast);
  ck_assert_str_eq(normalized.name.c_str(), fixture.name.c_str());
  ck_assert_str_eq(normalized.node_kind.c_str(), fixture.kind.c_str());
  ck_assert_uint_eq(as_uint(normalized.args.size()), as_uint(fixture.args.size()));
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
    } else {
      ck_assert_msg(!shell_ast.errors.empty(), "expected shell parse error for %s",
                    fixture.input.c_str());
      ck_assert_msg(!reusable.ast.errors.empty(), "expected reusable parse error for %s",
                    fixture.input.c_str());
      ck_assert_str_eq(shell_ast.errors[0].message.c_str(), fixture.error.c_str());
      ck_assert_str_eq(reusable.ast.errors[0].message.c_str(), fixture.error.c_str());
      ck_assert_str_eq(shell_ast.errors[0].message.c_str(),
                       reusable.ast.errors[0].message.c_str());
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
