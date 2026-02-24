#include "parser/python_parser.h"

#include "parser/parser.h"
#include "parser/tokenizer.h"

#include <cctype>

namespace iris::parser {

namespace {

void push_error(std::vector<ParseError>& errors, std::string message,
                std::size_t offset, std::size_t line, std::size_t column) {
  ParseError err;
  err.message = std::move(message);
  err.offset = offset;
  err.line = line;
  err.column = column;
  errors.push_back(std::move(err));
}

std::string_view strip_comment(std::string_view line) {
  bool in_string = false;
  char quote = '\0';
  bool escape = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (escape) {
      escape = false;
      continue;
    }
    if (in_string && c == '\\') {
      escape = true;
      continue;
    }
    if ((c == '\'' || c == '"')) {
      if (!in_string) {
        in_string = true;
        quote = c;
      } else if (c == quote) {
        in_string = false;
        quote = '\0';
      }
      continue;
    }
    if (!in_string && c == '#') {
      return line.substr(0, i);
    }
  }
  return line;
}

TokenizeResult tokenize_python(std::string_view input) {
  TokenizeResult out;
  std::vector<std::size_t> indent_stack{0};
  std::size_t index = 0;
  std::size_t line_num = 1;

  Tokenizer tokenizer("()=,:" );

  auto push_token = [&](TokenKind kind, std::string text, std::size_t offset,
                        std::size_t column, std::size_t length) {
    Token tok;
    tok.kind = kind;
    tok.text = std::move(text);
    tok.span.offset = offset;
    tok.span.line = line_num;
    tok.span.column = column;
    tok.span.length = length;
    out.tokens.push_back(std::move(tok));
  };

  while (index <= input.size()) {
    std::size_t line_start = index;
    while (index < input.size() && input[index] != '\n') {
      ++index;
    }
    std::size_t line_end = index;
    if (index < input.size() && input[index] == '\n') {
      ++index;
    }

    std::string_view line = input.substr(line_start, line_end - line_start);

    std::size_t indent = 0;
    while (indent < line.size() && line[indent] == ' ') {
      ++indent;
    }
    if (indent < line.size() && line[indent] == '\t') {
      push_error(out.errors, "tabs are not supported", line_start + indent, line_num,
                 indent + 1);
    }

    std::string_view trimmed = line.substr(indent);
    trimmed = strip_comment(trimmed);
    std::size_t non_ws = 0;
    while (non_ws < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[non_ws]))) {
      ++non_ws;
    }
    if (non_ws >= trimmed.size()) {
      ++line_num;
      continue;
    }

    if (indent > indent_stack.back()) {
      indent_stack.push_back(indent);
      push_token(TokenKind::Indent, "", line_start, 1, 0);
    } else {
      while (indent < indent_stack.back()) {
        indent_stack.pop_back();
        push_token(TokenKind::Dedent, "", line_start, 1, 0);
      }
      if (indent != indent_stack.back()) {
        push_error(out.errors, "inconsistent indentation", line_start, line_num, 1);
      }
    }

    std::string_view content = trimmed.substr(non_ws);
    if (!content.empty()) {
      auto tokens = tokenizer.tokenize(content);
      for (const auto& tok : tokens.tokens) {
        if (tok.kind == TokenKind::End) continue;
        Token adjusted = tok;
        adjusted.span.offset = line_start + indent + non_ws + tok.span.offset;
        adjusted.span.line = line_num;
        adjusted.span.column = indent + non_ws + tok.span.column;
        out.tokens.push_back(std::move(adjusted));
      }
      for (const auto& err : tokens.errors) {
        ParseError adjusted = err;
        adjusted.offset = line_start + indent + non_ws + err.offset;
        adjusted.line = line_num;
        adjusted.column = indent + non_ws + err.column;
        out.errors.push_back(std::move(adjusted));
      }
    }

    push_token(TokenKind::Newline, "", line_end, line.size() + 1, 0);
    ++line_num;
  }

  while (indent_stack.size() > 1) {
    indent_stack.pop_back();
    push_token(TokenKind::Dedent, "", index, 1, 0);
  }

  Token end;
  end.kind = TokenKind::End;
  end.text = "";
  end.span.offset = input.size();
  end.span.line = line_num;
  end.span.column = 1;
  end.span.length = 0;
  out.tokens.push_back(std::move(end));
  return out;
}

std::optional<Token> expect_identifier(ParserCursor& cursor, std::vector<ParseError>& errors,
                                       std::string message) {
  const Token& tok = cursor.peek();
  if (tok.kind == TokenKind::Identifier) {
    cursor.advance();
    return tok;
  }
  push_error(errors, std::move(message), tok.span.offset, tok.span.line, tok.span.column);
  return std::nullopt;
}

std::optional<PyExpr> parse_expr(ParserCursor& cursor, std::vector<ParseError>& errors) {
  const Token& tok = cursor.peek();
  if (tok.kind == TokenKind::Identifier || tok.kind == TokenKind::Number ||
      tok.kind == TokenKind::String) {
    PyExpr expr;
    expr.value = tok.text;
    if (tok.kind == TokenKind::Identifier) expr.kind = PyExpr::Kind::Identifier;
    if (tok.kind == TokenKind::Number) expr.kind = PyExpr::Kind::Number;
    if (tok.kind == TokenKind::String) expr.kind = PyExpr::Kind::String;
    expr.span = tok.span;
    cursor.advance();
    return expr;
  }
  push_error(errors, "expected expression", tok.span.offset, tok.span.line, tok.span.column);
  return std::nullopt;
}

void consume_newlines(ParserCursor& cursor) {
  while (!cursor.at_end()) {
    const Token& tok = cursor.peek();
    if (tok.kind == TokenKind::Newline) {
      cursor.advance();
      continue;
    }
    break;
  }
}

std::optional<PyStmt> parse_statement(ParserCursor& cursor, std::vector<ParseError>& errors);

std::vector<PyStmt> parse_block(ParserCursor& cursor, std::vector<ParseError>& errors) {
  std::vector<PyStmt> statements;
  while (!cursor.at_end()) {
    const Token& tok = cursor.peek();
    if (tok.kind == TokenKind::Dedent || tok.kind == TokenKind::End) break;
    auto stmt = parse_statement(cursor, errors);
    if (stmt.has_value()) {
      statements.push_back(std::move(*stmt));
      continue;
    }
    if (!cursor.at_end()) cursor.advance();
  }
  return statements;
}

std::optional<PyStmt> parse_statement(ParserCursor& cursor, std::vector<ParseError>& errors) {
  consume_newlines(cursor);
  const Token& tok = cursor.peek();
  if (tok.kind == TokenKind::End || tok.kind == TokenKind::Dedent) return std::nullopt;

  if (tok.kind == TokenKind::Identifier && tok.text == "def") {
    std::size_t start_offset = tok.span.offset;
    std::size_t start_line = tok.span.line;
    std::size_t start_col = tok.span.column;
    cursor.advance();
    auto name = expect_identifier(cursor, errors, "expected function name");
    if (!name.has_value()) return std::nullopt;
    if (!cursor.match_symbol("(").has_value()) {
      push_error(errors, "expected '(' after function name", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
      return std::nullopt;
    }
    if (!cursor.match_symbol(")").has_value()) {
      push_error(errors, "expected ')' after function parameters", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
      return std::nullopt;
    }
    if (!cursor.match_symbol(":").has_value()) {
      push_error(errors, "expected ':' after function signature", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
      return std::nullopt;
    }
    if (!cursor.match(TokenKind::Newline).has_value()) {
      push_error(errors, "expected newline after function signature", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
      return std::nullopt;
    }
    if (!cursor.match(TokenKind::Indent).has_value()) {
      push_error(errors, "expected indented block", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
      return std::nullopt;
    }

    PyStmt stmt;
    stmt.kind = PyStmt::Kind::FunctionDef;
    stmt.name = name->text;
    stmt.span.offset = start_offset;
    stmt.span.line = start_line;
    stmt.span.column = start_col;

    stmt.body = parse_block(cursor, errors);
    if (!cursor.match(TokenKind::Dedent).has_value()) {
      push_error(errors, "expected dedent after block", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
    }
    stmt.span.length = cursor.peek().span.offset - start_offset;
    return stmt;
  }

  if (tok.kind == TokenKind::Identifier && tok.text == "pass") {
    PyStmt stmt;
    stmt.kind = PyStmt::Kind::Pass;
    stmt.span = tok.span;
    cursor.advance();
    cursor.match(TokenKind::Newline);
    return stmt;
  }

  if (tok.kind == TokenKind::Identifier) {
    std::size_t start_offset = tok.span.offset;
    std::size_t start_line = tok.span.line;
    std::size_t start_col = tok.span.column;
    std::string name = tok.text;
    cursor.advance();
    if (!cursor.match_symbol("=").has_value()) {
      push_error(errors, "expected '=' in assignment", cursor.peek().span.offset,
                 cursor.peek().span.line, cursor.peek().span.column);
      return std::nullopt;
    }
    auto expr = parse_expr(cursor, errors);
    cursor.match(TokenKind::Newline);

    PyStmt stmt;
    stmt.kind = PyStmt::Kind::Assign;
    stmt.name = std::move(name);
    stmt.value = std::move(expr);
    stmt.span.offset = start_offset;
    stmt.span.line = start_line;
    stmt.span.column = start_col;
    stmt.span.length = cursor.peek().span.offset - start_offset;
    return stmt;
  }

  push_error(errors, "unexpected token", tok.span.offset, tok.span.line, tok.span.column);
  return std::nullopt;
}

} // namespace

PyParseResult parse_python(std::string_view input) {
  auto tokens = tokenize_python(input);
  ParserCursor cursor(std::move(tokens.tokens));

  PyParseResult out;
  out.errors = std::move(tokens.errors);

  while (!cursor.at_end()) {
    auto stmt = parse_statement(cursor, out.errors);
    if (stmt.has_value()) {
      out.statements.push_back(std::move(*stmt));
      continue;
    }
    if (!cursor.at_end()) cursor.advance();
  }

  const auto& cursor_errors = cursor.errors();
  out.errors.insert(out.errors.end(), cursor_errors.begin(), cursor_errors.end());
  return out;
}

} // namespace iris::parser
