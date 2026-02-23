#pragma once

#include "parser/tokenizer.h"

#include <optional>
#include <string>
#include <vector>

namespace iris::parser {

class ParserCursor {
public:
  explicit ParserCursor(std::vector<Token> tokens);

  const Token& peek(std::size_t lookahead = 0) const;
  const Token& advance();
  bool at_end() const;

  std::optional<Token> match(TokenKind kind);
  std::optional<Token> match_symbol(std::string_view symbol);

  std::optional<Token> expect(TokenKind kind, std::string message);
  std::optional<Token> expect_symbol(std::string_view symbol, std::string message);

  const std::vector<ParseError>& errors() const { return errors_; }

private:
  void push_error(const Token& tok, std::string message);

  std::vector<Token> tokens_;
  std::size_t index_{0};
  std::vector<ParseError> errors_{};
};

} // namespace iris::parser
