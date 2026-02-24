#pragma once

#include "parser/types.h"

#include <optional>
#include <string_view>
#include <vector>

namespace iris::parser {

struct JsonParseResult {
  std::optional<ValueNode> value;
  std::vector<ParseError> errors;
};

JsonParseResult parse_json(std::string_view input);

} // namespace iris::parser
