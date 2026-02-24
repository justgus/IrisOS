#pragma once

#include "parser/types.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iris::parser {

struct CppParam {
  std::string type;
  std::string name;
  Span span;
};

struct CppDecl {
  enum class Kind {
    Variable,
    Function
  };

  Kind kind{Kind::Variable};
  std::string type;
  std::string name;
  std::vector<CppParam> params;
  std::optional<std::string> initializer;
  Span span;
};

struct CppParseResult {
  std::vector<CppDecl> decls;
  std::vector<ParseError> errors;
};

CppParseResult parse_cpp(std::string_view input);

} // namespace iris::parser
