#include "parser/json_parser.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace iris::parser {

namespace {

struct Cursor {
  std::string_view input;
  std::size_t index{0};
  std::size_t line{1};
  std::size_t column{1};
  std::vector<ParseError> errors;

  bool eof() const { return index >= input.size(); }
  char peek() const { return eof() ? '\0' : input[index]; }

  char advance() {
    if (eof()) return '\0';
    char c = input[index++];
    if (c == '\n') {
      ++line;
      column = 1;
    } else {
      ++column;
    }
    return c;
  }

  void skip_ws() {
    while (!eof()) {
      char c = peek();
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
        advance();
        continue;
      }
      break;
    }
  }

  void push_error(std::string message, std::size_t err_offset,
                  std::size_t err_line, std::size_t err_col) {
    ParseError err;
    err.message = std::move(message);
    err.offset = err_offset;
    err.line = err_line;
    err.column = err_col;
    errors.push_back(std::move(err));
  }

  void error_here(std::string message) {
    push_error(std::move(message), index, line, column);
  }
};

static void append_utf8(std::string& out, std::uint32_t codepoint) {
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

static bool read_hex_quad(Cursor& cursor, std::uint32_t* out) {
  std::uint32_t value = 0;
  for (int i = 0; i < 4; ++i) {
    char c = cursor.peek();
    if (cursor.eof()) return false;
    cursor.advance();
    value <<= 4;
    if (c >= '0' && c <= '9') {
      value |= static_cast<std::uint32_t>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      value |= static_cast<std::uint32_t>(10 + (c - 'a'));
    } else if (c >= 'A' && c <= 'F') {
      value |= static_cast<std::uint32_t>(10 + (c - 'A'));
    } else {
      return false;
    }
  }
  *out = value;
  return true;
}

static std::optional<ValueNode> parse_value(Cursor& cursor);

static std::optional<std::string> parse_string(Cursor& cursor) {
  if (cursor.peek() != '"') return std::nullopt;
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;
  cursor.advance();

  std::string out;
  while (!cursor.eof()) {
    char c = cursor.advance();
    if (c == '"') return out;
    if (c == '\\') {
      if (cursor.eof()) {
        cursor.push_error("unterminated string", start_offset, start_line, start_col);
        return std::nullopt;
      }
      char esc = cursor.advance();
      switch (esc) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case 'u': {
          std::uint32_t codepoint = 0;
          if (!read_hex_quad(cursor, &codepoint)) {
            cursor.push_error("invalid unicode escape", start_offset, start_line, start_col);
            return std::nullopt;
          }
          if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
            std::size_t surrogate_offset = cursor.index;
            if (cursor.peek() != '\\') {
              cursor.push_error("expected unicode surrogate pair", surrogate_offset,
                                cursor.line, cursor.column);
              return std::nullopt;
            }
            cursor.advance();
            if (cursor.peek() != 'u') {
              cursor.push_error("expected unicode surrogate pair", surrogate_offset,
                                cursor.line, cursor.column);
              return std::nullopt;
            }
            cursor.advance();
            std::uint32_t low = 0;
            if (!read_hex_quad(cursor, &low)) {
              cursor.push_error("invalid unicode escape", surrogate_offset,
                                cursor.line, cursor.column);
              return std::nullopt;
            }
            if (low < 0xDC00 || low > 0xDFFF) {
              cursor.push_error("invalid unicode surrogate", surrogate_offset,
                                cursor.line, cursor.column);
              return std::nullopt;
            }
            std::uint32_t combined = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
            append_utf8(out, combined);
          } else {
            append_utf8(out, codepoint);
          }
          break;
        }
        default:
          cursor.push_error("invalid escape sequence", start_offset, start_line, start_col);
          return std::nullopt;
      }
      continue;
    }
    if (c == '\n') {
      cursor.push_error("unterminated string", start_offset, start_line, start_col);
      return std::nullopt;
    }
    out.push_back(c);
  }

  cursor.push_error("unterminated string", start_offset, start_line, start_col);
  return std::nullopt;
}

static std::optional<double> parse_number(Cursor& cursor) {
  std::size_t start = cursor.index;
  if (cursor.peek() == '-') cursor.advance();

  if (cursor.peek() == '0') {
    cursor.advance();
  } else if (std::isdigit(static_cast<unsigned char>(cursor.peek()))) {
    while (std::isdigit(static_cast<unsigned char>(cursor.peek()))) cursor.advance();
  } else {
    return std::nullopt;
  }

  if (cursor.peek() == '.') {
    cursor.advance();
    if (!std::isdigit(static_cast<unsigned char>(cursor.peek()))) return std::nullopt;
    while (std::isdigit(static_cast<unsigned char>(cursor.peek()))) cursor.advance();
  }

  if (cursor.peek() == 'e' || cursor.peek() == 'E') {
    cursor.advance();
    if (cursor.peek() == '+' || cursor.peek() == '-') cursor.advance();
    if (!std::isdigit(static_cast<unsigned char>(cursor.peek()))) return std::nullopt;
    while (std::isdigit(static_cast<unsigned char>(cursor.peek()))) cursor.advance();
  }

  std::string text(cursor.input.substr(start, cursor.index - start));
  char* end = nullptr;
  double value = std::strtod(text.c_str(), &end);
  if (!end || *end != '\0') return std::nullopt;
  return value;
}

static bool match_literal(Cursor& cursor, std::string_view literal) {
  for (char c : literal) {
    if (cursor.peek() != c) return false;
    cursor.advance();
  }
  return true;
}

static std::optional<ValueNode> parse_array(Cursor& cursor) {
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;
  cursor.advance();
  cursor.skip_ws();

  ValueArray items;
  if (cursor.peek() == ']') {
    cursor.advance();
    return ValueNode{Value{std::move(items)}, Span{start_offset, start_line, start_col,
                                                   cursor.index - start_offset}};
  }

  while (true) {
    cursor.skip_ws();
    auto value = parse_value(cursor);
    if (!value.has_value()) return std::nullopt;
    items.push_back(std::move(value.value()));
    cursor.skip_ws();
    if (cursor.peek() == ']') {
      cursor.advance();
      break;
    }
    if (cursor.peek() != ',') {
      cursor.error_here("expected ',' or ']'");
      return std::nullopt;
    }
    cursor.advance();
  }

  return ValueNode{Value{std::move(items)}, Span{start_offset, start_line, start_col,
                                                 cursor.index - start_offset}};
}

static std::optional<ValueNode> parse_object(Cursor& cursor) {
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;
  cursor.advance();
  cursor.skip_ws();

  ValueObject object;
  if (cursor.peek() == '}') {
    cursor.advance();
    return ValueNode{Value{std::move(object)}, Span{start_offset, start_line, start_col,
                                                    cursor.index - start_offset}};
  }

  while (true) {
    cursor.skip_ws();
    auto key = parse_string(cursor);
    if (!key.has_value()) {
      cursor.error_here("expected string key");
      return std::nullopt;
    }
    cursor.skip_ws();
    if (cursor.peek() != ':') {
      cursor.error_here("expected ':'");
      return std::nullopt;
    }
    cursor.advance();
    cursor.skip_ws();
    auto value = parse_value(cursor);
    if (!value.has_value()) return std::nullopt;
    object[*key] = std::move(value.value());
    cursor.skip_ws();
    if (cursor.peek() == '}') {
      cursor.advance();
      break;
    }
    if (cursor.peek() != ',') {
      cursor.error_here("expected ',' or '}'");
      return std::nullopt;
    }
    cursor.advance();
  }

  return ValueNode{Value{std::move(object)}, Span{start_offset, start_line, start_col,
                                                  cursor.index - start_offset}};
}

static std::optional<ValueNode> parse_value(Cursor& cursor) {
  cursor.skip_ws();
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;

  char c = cursor.peek();
  if (c == '{') {
    return parse_object(cursor);
  }
  if (c == '[') {
    return parse_array(cursor);
  }
  if (c == '"') {
    auto str = parse_string(cursor);
    if (!str.has_value()) return std::nullopt;
    ValueNode node;
    node.value = Value{*str};
    node.span = Span{start_offset, start_line, start_col, cursor.index - start_offset};
    return node;
  }
  if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
    auto num = parse_number(cursor);
    if (!num.has_value()) {
      cursor.error_here("invalid number");
      return std::nullopt;
    }
    ValueNode node;
    node.value = Value{*num};
    node.span = Span{start_offset, start_line, start_col, cursor.index - start_offset};
    return node;
  }
  if (match_literal(cursor, "true")) {
    ValueNode node;
    node.value = Value{true};
    node.span = Span{start_offset, start_line, start_col, cursor.index - start_offset};
    return node;
  }
  if (match_literal(cursor, "false")) {
    ValueNode node;
    node.value = Value{false};
    node.span = Span{start_offset, start_line, start_col, cursor.index - start_offset};
    return node;
  }
  if (match_literal(cursor, "null")) {
    ValueNode node;
    node.value = Value{std::monostate{}};
    node.span = Span{start_offset, start_line, start_col, cursor.index - start_offset};
    return node;
  }

  cursor.error_here("unexpected token");
  return std::nullopt;
}

} // namespace

JsonParseResult parse_json(std::string_view input) {
  Cursor cursor{input, 0, 1, 1, {}};
  cursor.skip_ws();

  JsonParseResult out;
  auto value = parse_value(cursor);
  if (value.has_value()) {
    cursor.skip_ws();
    if (!cursor.eof()) {
      cursor.error_here("trailing characters");
    }
    out.value = std::move(value);
  }
  out.errors = std::move(cursor.errors);
  return out;
}

} // namespace iris::parser
