#include "parser/cpp_parser.h"

#include "parser/parser.h"
#include "parser/tokenizer.h"

namespace iris::parser {

namespace {

std::optional<std::size_t> last_identifier_index(const std::vector<Token>& tokens) {
  for (std::size_t i = tokens.size(); i-- > 0;) {
    if (tokens[i].kind == TokenKind::Identifier) return i;
  }
  return std::nullopt;
}

std::string format_type(const std::vector<Token>& tokens, std::size_t end) {
  std::string out;
  for (std::size_t i = 0; i < end; ++i) {
    const Token& tok = tokens[i];
    if (out.empty()) {
      out = tok.text;
      continue;
    }
    if (tok.text == "::") {
      out += "::";
      continue;
    }
    if (tok.kind == TokenKind::Symbol && (tok.text == "*" || tok.text == "&")) {
      out += tok.text;
      continue;
    }
    out += " ";
    out += tok.text;
  }
  return out;
}

bool is_decl_delimiter(const Token& tok) {
  if (tok.kind != TokenKind::Symbol) return false;
  return tok.text == "(" || tok.text == "=" || tok.text == ";";
}

void push_error(std::vector<ParseError>& errors, const Token& tok, std::string message) {
  ParseError err;
  err.message = std::move(message);
  err.offset = tok.span.offset;
  err.line = tok.span.line;
  err.column = tok.span.column;
  errors.push_back(std::move(err));
}

std::optional<CppParam> parse_param(ParserCursor& cursor, std::vector<ParseError>& errors) {
  std::vector<Token> tokens;
  std::size_t start_offset = cursor.peek().span.offset;
  std::size_t start_line = cursor.peek().span.line;
  std::size_t start_col = cursor.peek().span.column;

  while (!cursor.at_end()) {
    const Token& tok = cursor.peek();
    if (tok.kind == TokenKind::Symbol && (tok.text == "," || tok.text == ")")) break;
    tokens.push_back(cursor.advance());
  }

  if (tokens.empty()) return std::nullopt;
  auto name_index = last_identifier_index(tokens);
  if (!name_index.has_value()) {
    push_error(errors, tokens.back(), "expected parameter name");
    return std::nullopt;
  }
  if (*name_index + 1 != tokens.size()) {
    push_error(errors, tokens.back(), "unexpected tokens after parameter name");
    return std::nullopt;
  }

  CppParam param;
  param.type = format_type(tokens, *name_index);
  param.name = tokens[*name_index].text;
  param.span.offset = start_offset;
  param.span.line = start_line;
  param.span.column = start_col;
  param.span.length = tokens.back().span.offset + tokens.back().span.length - start_offset;
  return param;
}

void skip_to_statement_end(ParserCursor& cursor) {
  while (!cursor.at_end()) {
    const Token& tok = cursor.peek();
    if (tok.kind == TokenKind::Symbol && tok.text == ";") {
      cursor.advance();
      return;
    }
    cursor.advance();
  }
}

std::optional<CppDecl> parse_decl(ParserCursor& cursor, std::vector<ParseError>& errors) {
  std::vector<Token> tokens;
  std::size_t start_offset = cursor.peek().span.offset;
  std::size_t start_line = cursor.peek().span.line;
  std::size_t start_col = cursor.peek().span.column;

  while (!cursor.at_end()) {
    const Token& tok = cursor.peek();
    if (is_decl_delimiter(tok)) break;
    if (tok.kind == TokenKind::Symbol && tok.text == "{") break;
    tokens.push_back(cursor.advance());
  }

  if (tokens.empty()) return std::nullopt;
  auto name_index = last_identifier_index(tokens);
  if (!name_index.has_value()) {
    push_error(errors, tokens.back(), "expected declaration name");
    skip_to_statement_end(cursor);
    return std::nullopt;
  }
  if (*name_index + 1 != tokens.size()) {
    push_error(errors, tokens.back(), "unexpected tokens after declaration name");
    skip_to_statement_end(cursor);
    return std::nullopt;
  }

  const Token& delimiter = cursor.peek();
  if (delimiter.kind != TokenKind::Symbol) {
    push_error(errors, delimiter, "expected declaration delimiter");
    skip_to_statement_end(cursor);
    return std::nullopt;
  }

  CppDecl decl;
  decl.type = format_type(tokens, *name_index);
  decl.name = tokens[*name_index].text;

  if (delimiter.text == "(") {
    cursor.advance();
    decl.kind = CppDecl::Kind::Function;
    if (cursor.peek().kind == TokenKind::Symbol && cursor.peek().text == ")") {
      cursor.advance();
    } else {
      while (!cursor.at_end()) {
        auto param = parse_param(cursor, errors);
        if (param.has_value()) decl.params.push_back(std::move(*param));
        if (cursor.peek().kind == TokenKind::Symbol && cursor.peek().text == ",") {
          cursor.advance();
          continue;
        }
        break;
      }
      if (!cursor.match_symbol(")").has_value()) {
        push_error(errors, cursor.peek(), "expected ')' after parameters");
        skip_to_statement_end(cursor);
        return std::nullopt;
      }
    }
    if (!cursor.match_symbol(";").has_value()) {
      push_error(errors, cursor.peek(), "expected ';' after declaration");
      skip_to_statement_end(cursor);
      return std::nullopt;
    }
  } else if (delimiter.text == "=") {
    cursor.advance();
    decl.kind = CppDecl::Kind::Variable;
    const Token& init = cursor.peek();
    if (init.kind == TokenKind::Identifier || init.kind == TokenKind::Number ||
        init.kind == TokenKind::String) {
      decl.initializer = init.text;
      cursor.advance();
    } else {
      push_error(errors, init, "expected initializer literal");
    }
    if (!cursor.match_symbol(";").has_value()) {
      push_error(errors, cursor.peek(), "expected ';' after initializer");
      skip_to_statement_end(cursor);
      return std::nullopt;
    }
  } else if (delimiter.text == ";") {
    cursor.advance();
    decl.kind = CppDecl::Kind::Variable;
  } else {
    push_error(errors, delimiter, "unsupported declaration delimiter");
    skip_to_statement_end(cursor);
    return std::nullopt;
  }

  decl.span.offset = start_offset;
  decl.span.line = start_line;
  decl.span.column = start_col;
  decl.span.length = cursor.peek().span.offset - start_offset;
  return decl;
}

} // namespace

CppParseResult parse_cpp(std::string_view input) {
  Tokenizer tokenizer("(){}[],:;=*<>&");
  auto result = tokenizer.tokenize(input);
  ParserCursor cursor(std::move(result.tokens));

  CppParseResult out;
  out.errors = std::move(result.errors);

  while (!cursor.at_end()) {
    if (cursor.peek().kind == TokenKind::End) break;
    auto decl = parse_decl(cursor, out.errors);
    if (decl.has_value()) out.decls.push_back(std::move(*decl));
    if (decl.has_value()) continue;
    if (!cursor.at_end()) cursor.advance();
  }

  const auto& cursor_errors = cursor.errors();
  out.errors.insert(out.errors.end(), cursor_errors.begin(), cursor_errors.end());
  return out;
}

} // namespace iris::parser
