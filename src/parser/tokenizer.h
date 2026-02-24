#pragma once

#include "parser/types.h"

#include <string>
#include <string_view>
#include <vector>

namespace iris::parser {

enum class TokenKind {
  Identifier,
  Number,
  String,
  Symbol,
  Newline,
  Indent,
  Dedent,
  End
};

struct Token {
  TokenKind kind{TokenKind::End};
  std::string text;
  Span span;
};

struct TokenizeResult {
  std::vector<Token> tokens;
  std::vector<ParseError> errors;
};

class Tokenizer {
public:
  explicit Tokenizer(std::string symbols = "(){}[],:=");

  TokenizeResult tokenize(std::string_view input) const;
  TokenizeResult tokenize_loose(std::string_view input) const;

private:
  bool is_symbol(char c) const;
  static bool is_ident_start(char c);
  static bool is_ident_char(char c);

  std::string symbols_;
};

} // namespace iris::parser
