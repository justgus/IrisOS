#include "parser/tokenizer.h"

#include <cctype>

namespace iris::parser {

Tokenizer::Tokenizer(std::string symbols) : symbols_(std::move(symbols)) {}

bool Tokenizer::is_symbol(char c) const {
  return symbols_.find(c) != std::string::npos;
}

bool Tokenizer::is_ident_start(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Tokenizer::is_ident_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

TokenizeResult Tokenizer::tokenize(std::string_view input) const {
  TokenizeResult out;
  std::size_t i = 0;
  std::size_t line = 1;
  std::size_t column = 1;

  auto push_token = [&](TokenKind kind, std::string text, std::size_t start_offset,
                        std::size_t start_line, std::size_t start_column) {
    Token tok;
    tok.kind = kind;
    tok.text = std::move(text);
    tok.span.offset = start_offset;
    tok.span.line = start_line;
    tok.span.column = start_column;
    tok.span.length = tok.text.size();
    out.tokens.push_back(std::move(tok));
  };

  auto push_error = [&](std::string message, std::size_t err_offset,
                        std::size_t err_line, std::size_t err_column) {
    ParseError err;
    err.message = std::move(message);
    err.offset = err_offset;
    err.line = err_line;
    err.column = err_column;
    out.errors.push_back(std::move(err));
  };

  while (i < input.size()) {
    char c = input[i];
    if (c == '\n') {
      ++i;
      ++line;
      column = 1;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(c))) {
      ++i;
      ++column;
      continue;
    }

    if (c == '/' && i + 1 < input.size() && input[i + 1] == '/') {
      i += 2;
      column += 2;
      while (i < input.size() && input[i] != '\n') {
        ++i;
        ++column;
      }
      continue;
    }

    if (is_ident_start(c)) {
      std::size_t start = i;
      std::size_t start_line = line;
      std::size_t start_col = column;
      ++i;
      ++column;
      while (i < input.size() && is_ident_char(input[i])) {
        ++i;
        ++column;
      }
      push_token(TokenKind::Identifier, std::string(input.substr(start, i - start)),
                 start, start_line, start_col);
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::size_t start = i;
      std::size_t start_line = line;
      std::size_t start_col = column;
      ++i;
      ++column;
      bool seen_dot = false;
      while (i < input.size()) {
        char d = input[i];
        if (std::isdigit(static_cast<unsigned char>(d))) {
          ++i;
          ++column;
          continue;
        }
        if (d == '.' && !seen_dot) {
          seen_dot = true;
          ++i;
          ++column;
          continue;
        }
        break;
      }
      push_token(TokenKind::Number, std::string(input.substr(start, i - start)),
                 start, start_line, start_col);
      continue;
    }

    if (c == '"') {
      std::size_t start = i;
      std::size_t start_line = line;
      std::size_t start_col = column;
      ++i;
      ++column;
      std::string value;
      bool closed = false;
      while (i < input.size()) {
        char d = input[i];
        if (d == '\n') {
          push_error("unterminated string", start, start_line, start_col);
          break;
        }
        if (d == '"') {
          closed = true;
          ++i;
          ++column;
          break;
        }
        if (d == '\\' && i + 1 < input.size()) {
          value.push_back(input[i + 1]);
          i += 2;
          column += 2;
          continue;
        }
        value.push_back(d);
        ++i;
        ++column;
      }
      if (closed) {
        push_token(TokenKind::String, value, start, start_line, start_col);
      } else if (i >= input.size()) {
        push_error("unterminated string", start, start_line, start_col);
      }
      continue;
    }

    if (is_symbol(c)) {
      std::size_t start = i;
      std::size_t start_line = line;
      std::size_t start_col = column;
      ++i;
      ++column;
      push_token(TokenKind::Symbol, std::string(1, c), start, start_line, start_col);
      continue;
    }

    push_error("unexpected character", i, line, column);
    ++i;
    ++column;
  }

  Token end;
  end.kind = TokenKind::End;
  end.text = "";
  end.span.offset = i;
  end.span.line = line;
  end.span.column = column;
  end.span.length = 0;
  out.tokens.push_back(std::move(end));

  return out;
}

TokenizeResult Tokenizer::tokenize_loose(std::string_view input) const {
  TokenizeResult out;
  std::size_t i = 0;
  std::size_t line = 1;
  std::size_t column = 1;

  auto push_token = [&](TokenKind kind, std::string text, std::size_t start_offset,
                        std::size_t start_line, std::size_t start_column) {
    Token tok;
    tok.kind = kind;
    tok.text = std::move(text);
    tok.span.offset = start_offset;
    tok.span.line = start_line;
    tok.span.column = start_column;
    tok.span.length = tok.text.size();
    out.tokens.push_back(std::move(tok));
  };

  auto push_error = [&](std::string message, std::size_t err_offset,
                        std::size_t err_line, std::size_t err_column) {
    ParseError err;
    err.message = std::move(message);
    err.offset = err_offset;
    err.line = err_line;
    err.column = err_column;
    out.errors.push_back(std::move(err));
  };

  while (i < input.size()) {
    char c = input[i];
    if (c == '\n') {
      ++i;
      ++line;
      column = 1;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(c))) {
      ++i;
      ++column;
      continue;
    }

    if (c == '\"') {
      std::size_t start = i;
      std::size_t start_line = line;
      std::size_t start_col = column;
      ++i;
      ++column;
      std::string value;
      bool closed = false;
      while (i < input.size()) {
        char d = input[i];
        if (d == '\n') {
          push_error("unterminated string", start, start_line, start_col);
          break;
        }
        if (d == '\"') {
          closed = true;
          ++i;
          ++column;
          break;
        }
        if (d == '\\' && i + 1 < input.size()) {
          value.push_back(input[i + 1]);
          i += 2;
          column += 2;
          continue;
        }
        value.push_back(d);
        ++i;
        ++column;
      }
      if (closed) {
        push_token(TokenKind::String, value, start, start_line, start_col);
      } else if (i >= input.size()) {
        push_error("unterminated string", start, start_line, start_col);
      }
      continue;
    }

    std::size_t start = i;
    std::size_t start_line = line;
    std::size_t start_col = column;
    while (i < input.size() && !std::isspace(static_cast<unsigned char>(input[i]))
           && input[i] != '\n') {
      ++i;
      ++column;
    }
    push_token(TokenKind::Identifier, std::string(input.substr(start, i - start)),
               start, start_line, start_col);
  }

  Token end;
  end.kind = TokenKind::End;
  end.text = "";
  end.span.offset = i;
  end.span.line = line;
  end.span.column = column;
  end.span.length = 0;
  out.tokens.push_back(std::move(end));

  return out;
}

} // namespace iris::parser
