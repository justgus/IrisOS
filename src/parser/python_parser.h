#pragma once

#include "parser/types.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iris::parser {

struct PyExpr {
  enum class Kind {
    Identifier,
    Number,
    String
  };

  Kind kind{Kind::Identifier};
  std::string value;
  Span span;
};

struct PyStmt {
  enum class Kind {
    Assign,
    Pass,
    FunctionDef
  };

  Kind kind{Kind::Assign};
  std::string name;
  std::optional<PyExpr> value;
  std::vector<PyStmt> body;
  Span span;
};

struct PyParseResult {
  std::vector<PyStmt> statements;
  std::vector<ParseError> errors;
};

PyParseResult parse_python(std::string_view input);

} // namespace iris::parser
