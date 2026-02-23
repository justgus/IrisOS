#pragma once

#include "parser/tokenizer.h"

#include <string>
#include <vector>

namespace iris::parser {

struct CommandAst {
  std::string name;
  std::vector<std::string> args;
  std::vector<ParseError> errors;
};

CommandAst parse_conch_command(std::string_view input);

} // namespace iris::parser
