#include "parser/xml_parser.h"

#include <cctype>

namespace iris::parser {

namespace {

struct Cursor {
  std::string_view input;
  std::size_t index{0};
  std::size_t line{1};
  std::size_t column{1};
  std::vector<ParseError> errors;

  bool eof() const { return index >= input.size(); }
  char peek(std::size_t lookahead = 0) const {
    std::size_t pos = index + lookahead;
    if (pos >= input.size()) return '\0';
    return input[pos];
  }

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

  bool match(char c) {
    if (peek() != c) return false;
    advance();
    return true;
  }

  bool match_sequence(std::string_view seq) {
    for (char c : seq) {
      if (peek() != c) return false;
      advance();
    }
    return true;
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

static bool is_name_start(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == ':';
}

static bool is_name_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':' || c == '-' || c == '.';
}

static std::optional<std::string> parse_name(Cursor& cursor) {
  if (!is_name_start(cursor.peek())) return std::nullopt;
  std::size_t start = cursor.index;
  while (!cursor.eof() && is_name_char(cursor.peek())) {
    cursor.advance();
  }
  return std::string(cursor.input.substr(start, cursor.index - start));
}

static bool skip_comment(Cursor& cursor) {
  if (cursor.peek() != '<' || cursor.peek(1) != '!' || cursor.peek(2) != '-' ||
      cursor.peek(3) != '-') {
    return false;
  }
  cursor.advance();
  cursor.advance();
  cursor.advance();
  cursor.advance();
  while (!cursor.eof()) {
    if (cursor.peek() == '-' && cursor.peek(1) == '-' && cursor.peek(2) == '>') {
      cursor.advance();
      cursor.advance();
      cursor.advance();
      return true;
    }
    cursor.advance();
  }
  cursor.error_here("unterminated comment");
  return true;
}

static bool skip_processing(Cursor& cursor) {
  if (cursor.peek() != '<' || cursor.peek(1) != '?') return false;
  cursor.advance();
  cursor.advance();
  while (!cursor.eof()) {
    if (cursor.peek() == '?' && cursor.peek(1) == '>') {
      cursor.advance();
      cursor.advance();
      return true;
    }
    cursor.advance();
  }
  cursor.error_here("unterminated processing instruction");
  return true;
}

static std::optional<std::string> parse_entity(Cursor& cursor) {
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;

  if (!cursor.match('&')) return std::nullopt;
  std::size_t name_start = cursor.index;
  while (!cursor.eof() && cursor.peek() != ';') {
    char c = cursor.peek();
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '#') {
      cursor.push_error("invalid entity reference", start_offset, start_line, start_col);
      return std::nullopt;
    }
    cursor.advance();
  }
  if (!cursor.match(';')) {
    cursor.push_error("unterminated entity reference", start_offset, start_line, start_col);
    return std::nullopt;
  }

  std::string_view entity = cursor.input.substr(name_start, cursor.index - name_start - 1);
  if (entity == "lt") return std::string("<");
  if (entity == "gt") return std::string(">");
  if (entity == "amp") return std::string("&");
  if (entity == "quot") return std::string("\"");
  if (entity == "apos") return std::string("'");

  cursor.push_error("unknown entity reference", start_offset, start_line, start_col);
  return std::string("?");
}

static std::optional<std::string> parse_attr_value(Cursor& cursor) {
  char quote = cursor.peek();
  if (quote != '"' && quote != '\'') return std::nullopt;
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;
  cursor.advance();

  std::string out;
  while (!cursor.eof()) {
    char c = cursor.peek();
    if (c == quote) {
      cursor.advance();
      return out;
    }
    if (c == '&') {
      auto ent = parse_entity(cursor);
      if (ent.has_value()) out.append(*ent);
      continue;
    }
    out.push_back(cursor.advance());
  }

  cursor.push_error("unterminated attribute value", start_offset, start_line, start_col);
  return std::nullopt;
}

static std::optional<XmlNode> parse_element(Cursor& cursor);

static std::optional<XmlNode> parse_text(Cursor& cursor) {
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;
  std::string out;
  while (!cursor.eof()) {
    char c = cursor.peek();
    if (c == '<') break;
    if (c == '&') {
      auto ent = parse_entity(cursor);
      if (ent.has_value()) out.append(*ent);
      continue;
    }
    out.push_back(cursor.advance());
  }

  if (out.empty()) return std::nullopt;

  XmlNode node;
  node.kind = XmlNode::Kind::Text;
  node.text = std::move(out);
  node.span.offset = start_offset;
  node.span.line = start_line;
  node.span.column = start_col;
  node.span.length = cursor.index - start_offset;
  return node;
}

static std::optional<XmlNode> parse_element(Cursor& cursor) {
  std::size_t start_offset = cursor.index;
  std::size_t start_line = cursor.line;
  std::size_t start_col = cursor.column;

  if (!cursor.match('<')) return std::nullopt;
  if (cursor.peek() == '/') {
    cursor.error_here("unexpected closing tag");
    return std::nullopt;
  }

  auto name = parse_name(cursor);
  if (!name.has_value()) {
    cursor.error_here("expected element name");
    return std::nullopt;
  }

  XmlNode node;
  node.kind = XmlNode::Kind::Element;
  node.name = std::move(*name);

  while (!cursor.eof()) {
    cursor.skip_ws();
    if (cursor.peek() == '/' || cursor.peek() == '>') break;

    std::size_t attr_offset = cursor.index;
    std::size_t attr_line = cursor.line;
    std::size_t attr_col = cursor.column;
    auto attr_name = parse_name(cursor);
    if (!attr_name.has_value()) {
      cursor.error_here("expected attribute name");
      break;
    }
    cursor.skip_ws();
    if (!cursor.match('=')) {
      cursor.error_here("expected '=' after attribute name");
      break;
    }
    cursor.skip_ws();
    auto value = parse_attr_value(cursor);
    if (!value.has_value()) {
      cursor.error_here("expected quoted attribute value");
      break;
    }

    XmlAttribute attr;
    attr.name = std::move(*attr_name);
    attr.value = std::move(*value);
    attr.span.offset = attr_offset;
    attr.span.line = attr_line;
    attr.span.column = attr_col;
    attr.span.length = cursor.index - attr_offset;
    node.attributes.push_back(std::move(attr));
  }

  cursor.skip_ws();
  if (cursor.match('/')) {
    if (!cursor.match('>')) cursor.error_here("expected '>' after '/'");
    node.span.offset = start_offset;
    node.span.line = start_line;
    node.span.column = start_col;
    node.span.length = cursor.index - start_offset;
    return node;
  }

  if (!cursor.match('>')) {
    cursor.error_here("expected '>' after element");
    return node;
  }

  while (!cursor.eof()) {
    if (skip_comment(cursor) || skip_processing(cursor)) {
      continue;
    }
    if (cursor.peek() == '<' && cursor.peek(1) == '/') {
      cursor.advance();
      cursor.advance();
      auto end_name = parse_name(cursor);
      if (!end_name.has_value()) {
        cursor.error_here("expected closing tag name");
        break;
      }
      cursor.skip_ws();
      if (!cursor.match('>')) cursor.error_here("expected '>' to close tag");
      if (*end_name != node.name) {
        cursor.error_here("mismatched closing tag");
      }
      break;
    }
    if (cursor.peek() == '<') {
      auto child = parse_element(cursor);
      if (child.has_value()) node.children.push_back(std::move(*child));
      continue;
    }
    auto text = parse_text(cursor);
    if (text.has_value()) node.children.push_back(std::move(*text));
  }

  node.span.offset = start_offset;
  node.span.line = start_line;
  node.span.column = start_col;
  node.span.length = cursor.index - start_offset;
  return node;
}

} // namespace

XmlParseResult parse_xml(std::string_view input) {
  Cursor cursor{input, 0, 1, 1, {}};

  while (!cursor.eof()) {
    cursor.skip_ws();
    if (skip_comment(cursor) || skip_processing(cursor)) continue;
    break;
  }

  XmlParseResult out;
  auto root = parse_element(cursor);
  if (root.has_value()) out.root = std::move(*root);

  cursor.skip_ws();
  while (!cursor.eof()) {
    if (skip_comment(cursor) || skip_processing(cursor)) {
      cursor.skip_ws();
      continue;
    }
    cursor.error_here("unexpected trailing content");
    break;
  }

  out.errors = std::move(cursor.errors);
  return out;
}

} // namespace iris::parser
