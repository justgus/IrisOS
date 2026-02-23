#include "parser/parser.h"

namespace iris::parser {

ParserCursor::ParserCursor(std::vector<Token> tokens)
  : tokens_(std::move(tokens)) {
  if (tokens_.empty()) {
    Token end;
    end.kind = TokenKind::End;
    tokens_.push_back(end);
  }
}

const Token& ParserCursor::peek(std::size_t lookahead) const {
  std::size_t idx = index_ + lookahead;
  if (idx >= tokens_.size()) return tokens_.back();
  return tokens_[idx];
}

const Token& ParserCursor::advance() {
  if (index_ < tokens_.size() - 1) ++index_;
  return tokens_[index_];
}

bool ParserCursor::at_end() const {
  return peek().kind == TokenKind::End;
}

std::optional<Token> ParserCursor::match(TokenKind kind) {
  if (peek().kind != kind) return std::nullopt;
  Token tok = peek();
  advance();
  return tok;
}

std::optional<Token> ParserCursor::match_symbol(std::string_view symbol) {
  const Token& tok = peek();
  if (tok.kind != TokenKind::Symbol || tok.text != symbol) return std::nullopt;
  Token out = tok;
  advance();
  return out;
}

std::optional<Token> ParserCursor::expect(TokenKind kind, std::string message) {
  auto tok = match(kind);
  if (tok.has_value()) return tok;
  push_error(peek(), std::move(message));
  return std::nullopt;
}

std::optional<Token> ParserCursor::expect_symbol(std::string_view symbol, std::string message) {
  auto tok = match_symbol(symbol);
  if (tok.has_value()) return tok;
  push_error(peek(), std::move(message));
  return std::nullopt;
}

void ParserCursor::push_error(const Token& tok, std::string message) {
  ParseError err;
  err.message = std::move(message);
  err.offset = tok.span.offset;
  err.line = tok.span.line;
  err.column = tok.span.column;
  errors_.push_back(std::move(err));
}

} // namespace iris::parser
