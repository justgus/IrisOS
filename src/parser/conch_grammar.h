#pragma once

#include "parser/conch_command.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iris::parser {

struct ConchParseResult {
  CommandAst ast{};
};

struct NormalizedCommand {
  std::string name;
  std::string node_kind;
  std::vector<std::string> args;
};

ConchParseResult parse_conch_grammar(std::string_view input);
NormalizedCommand normalize_command(const CommandAst& ast);
std::string command_node_kind(const CommandAst& ast);

} // namespace iris::parser
