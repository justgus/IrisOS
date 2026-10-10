#include "parser/conch_command.h"
#include "parser/json_parser.h"

#include <charconv>
#include <cctype>
#include <limits>
#include <string_view>

namespace iris::parser {

namespace {

std::string trim_copy(std::string_view input) {
  std::size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
    ++start;
  }

  std::size_t end = input.size();
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
    --end;
  }

  return std::string(input.substr(start, end - start));
}

bool is_identifier(std::string_view text) {
  if (text.empty()) return false;
  auto first = static_cast<unsigned char>(text.front());
  if (!(std::isalpha(first) || first == '_')) return false;
  for (char c : text.substr(1)) {
    auto value = static_cast<unsigned char>(c);
    if (!(std::isalnum(value) || c == '_' || c == ':' || c == '.')) return false;
  }
  return true;
}

ParseError located_error(std::string_view input, std::size_t offset, std::string message) {
  std::size_t line = 1;
  std::size_t column = 1;
  offset = std::min(offset, input.size());
  for (std::size_t i = 0; i < offset; ++i) {
    if (input[i] == '\n') { ++line; column = 1; }
    else ++column;
  }
  return ParseError{std::move(message), offset, line, column};
}

void adjust_value_spans(ValueNode* node, std::string_view source, std::size_t content_offset) {
  if (!node) return;
  auto location = located_error(source, content_offset + node->span.offset, {});
  node->span.offset = location.offset;
  node->span.line = location.line;
  node->span.column = location.column;
  if (auto array = std::get_if<ValueArray>(&node->value)) {
    for (auto& child : *array) adjust_value_spans(&child, source, content_offset);
  } else if (auto object = std::get_if<ValueObject>(&node->value)) {
    for (auto& [key, child] : *object) {
      (void)key;
      adjust_value_spans(&child, source, content_offset);
    }
  }
}

std::string_view tail_after_token(std::string_view input, const Token& token) {
  auto start = token.span.offset + token.span.length;
  if (start >= input.size()) return {};
  return input.substr(start);
}

std::optional<ValueNode> parse_inline_value(std::string_view text, std::size_t offset,
                                             std::size_t line, std::size_t column,
                                             ParseError* error) {
  if (text.empty()) {
    if (error) *error = ParseError{"expected field:=value", offset, line, column};
    return std::nullopt;
  }
  if (text.front() == '\'' || text.front() == '"') {
    char quote = text.front();
    std::string value;
    std::size_t i = 1;
    for (; i < text.size(); ++i) {
      char c = text[i];
      if (c == quote) break;
      if (c == '\\' && i + 1 < text.size()) c = text[++i];
      value.push_back(c);
    }
    if (i + 1 != text.size()) {
      if (error) *error = ParseError{"unterminated quoted value", offset, line, column};
      return std::nullopt;
    }
    return ValueNode{Value{std::move(value)}, Span{offset, line, column, text.size()}};
  }
  if (text == "true") return ValueNode{Value{true}, Span{offset, line, column, text.size()}};
  if (text == "false") return ValueNode{Value{false}, Span{offset, line, column, text.size()}};
  if (text.front() == '-' || std::isdigit(static_cast<unsigned char>(text.front()))) {
    std::int64_t number = 0;
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
    if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()) {
      return ValueNode{Value{number}, Span{offset, line, column, text.size()}};
    }
    if (error) *error = ParseError{"invalid signed 64-bit integer", offset, line, column};
    return std::nullopt;
  }
  return ValueNode{Value{std::string(text)}, Span{offset, line, column, text.size()}};
}

std::optional<AuthoringCommand> parse_new_authoring(std::string_view input,
                                                    std::vector<ParseError>* errors) {
  const auto source = input;
  std::size_t start = input.find_first_not_of(" \t\r\n");
  if (start == std::string_view::npos) return std::nullopt;
  input.remove_prefix(start);
  if (input.substr(0, 3) != "new" || (input.size() > 3 && !std::isspace(input[3]))) {
    return std::nullopt;
  }
  input.remove_prefix(3);
  std::size_t lead = input.find_first_not_of(" \t\r\n");
  if (lead == std::string_view::npos) {
    errors->push_back(located_error(source, start + 3,
                                    "expected type name or --json payload"));
    return std::nullopt;
  }
  input.remove_prefix(lead);
  const std::size_t payload_offset = start + 3 + lead;
  AuthoringCommand command;
  command.payload = ValueNode{Value{ValueObject{}}, Span{}};
  if (input.substr(0, 6) == "--json" && (input.size() == 6 || std::isspace(input[6]))) {
    command.json_mode = true;
    input.remove_prefix(6);
    std::size_t json_start = input.find_first_not_of(" \t\r\n");
    if (json_start == std::string_view::npos) {
      errors->push_back(located_error(source, payload_offset + 6, "expected JSON payload"));
      return std::nullopt;
    }
    input.remove_prefix(json_start);
    std::string json_text(input);
    std::size_t wrapper_offset = 0;
    if (json_text.size() >= 2 && ((json_text.front() == '\'' && json_text.back() == '\'') ||
                                  (json_text.front() == '"' && json_text.back() == '"'))) {
      wrapper_offset = 1;
      json_text = json_text.substr(1, json_text.size() - 2);
    }
    auto parsed = parse_json(json_text);
    if (!parsed.errors.empty() || !parsed.value ||
        !std::holds_alternative<ValueObject>(parsed.value->value)) {
      if (!parsed.errors.empty()) {
        errors->push_back(located_error(source, payload_offset + 6 + json_start +
                                        wrapper_offset + parsed.errors.front().offset,
                                        parsed.errors.front().message));
      } else {
        errors->push_back(located_error(source, payload_offset + 6 + json_start,
                                        "JSON payload must be an object"));
      }
      return std::nullopt;
    }
    adjust_value_spans(&*parsed.value, source,
                       payload_offset + 6 + json_start + wrapper_offset);
    const auto& root = std::get<ValueObject>(parsed.value->value);
    auto type = root.find("type");
    if (type == root.end() || !std::holds_alternative<std::string>(type->second.value) ||
        std::get<std::string>(type->second.value).empty()) {
      errors->push_back(located_error(source, payload_offset + 6 + json_start,
                                      "json missing type"));
      return std::nullopt;
    }
    command.type_name = std::get<std::string>(type->second.value);
    if (!is_identifier(command.type_name)) {
      errors->push_back(located_error(source, payload_offset + 6 + json_start,
                                      "invalid type name"));
      return std::nullopt;
    }
    auto payload = root.find("payload");
    if (payload != root.end()) command.payload = payload->second;
    return command;
  }

  std::size_t type_end = input.find_first_of(" \t\r\n");
  command.type_name = std::string(input.substr(0, type_end));
  if (!is_identifier(command.type_name)) {
    errors->push_back(located_error(source, payload_offset, "invalid type name"));
    return std::nullopt;
  }
  if (type_end == std::string_view::npos) return command;
  std::string_view rest = input.substr(type_end);
  std::size_t pos = 0;
  ValueObject fields;
  while (pos < rest.size()) {
    while (pos < rest.size() && std::isspace(static_cast<unsigned char>(rest[pos]))) ++pos;
    if (pos == rest.size()) break;
    std::size_t name_start = pos;
    while (pos < rest.size() && !std::isspace(static_cast<unsigned char>(rest[pos])) &&
           rest.substr(pos, 2) != ":=") ++pos;
    std::string name(rest.substr(name_start, pos - name_start));
    if (!is_identifier(name)) {
      errors->push_back(located_error(source, payload_offset + type_end + name_start,
                                      "invalid field name"));
      return std::nullopt;
    }
    while (pos < rest.size() && std::isspace(static_cast<unsigned char>(rest[pos]))) ++pos;
    if (pos >= rest.size() || rest.substr(pos, 2) != ":=" || name.empty()) {
      errors->push_back(located_error(source, payload_offset + type_end + pos,
                                      "expected field:=value"));
      return std::nullopt;
    }
    pos += 2;
    while (pos < rest.size() && std::isspace(static_cast<unsigned char>(rest[pos]))) ++pos;
    std::size_t value_start = pos;
    if (pos < rest.size() && (rest[pos] == '\'' || rest[pos] == '"')) {
      char quote = rest[pos++];
      while (pos < rest.size()) {
        if (rest[pos] == '\\' && pos + 1 < rest.size()) { pos += 2; continue; }
        if (rest[pos++] == quote) break;
      }
    } else {
      while (pos < rest.size() && !std::isspace(static_cast<unsigned char>(rest[pos]))) ++pos;
    }
    ParseError error;
    const auto absolute_value_offset = payload_offset + type_end + value_start;
    const auto value_location = located_error(source, absolute_value_offset, {});
    auto value = parse_inline_value(rest.substr(value_start, pos - value_start),
                                    absolute_value_offset, value_location.line,
                                    value_location.column,
                                    &error);
    if (!value) {
      errors->push_back(located_error(source, absolute_value_offset, error.message));
      return std::nullopt;
    }
    fields[std::move(name)] = std::move(*value);
  }
  command.payload = ValueNode{Value{std::move(fields)}, Span{}};
  return command;
}

std::optional<AuthoringCommand> parse_define_authoring(std::string_view input,
                                                       std::vector<ParseError>* errors) {
  const auto source = input;
  const auto command_start = input.find_first_not_of(" \t\r\n");
  auto trimmed = trim_copy(input);
  std::string_view rest(trimmed);
  auto first_end = rest.find_first_of(" \t\r\n");
  if (rest.substr(0, first_end) != "define" || first_end == std::string_view::npos) {
    return std::nullopt;
  }
  rest.remove_prefix(first_end);
  std::size_t lead = rest.find_first_not_of(" \t\r\n");
  if (lead == std::string_view::npos) {
    errors->push_back(located_error(source, command_start + first_end,
                                    "expected type name or --json payload"));
    return std::nullopt;
  }
  rest.remove_prefix(lead);
  const auto type_word_start = command_start + first_end + lead;
  auto type_end = rest.find_first_of(" \t\r\n");
  if (rest.substr(0, type_end) != "type" || type_end == std::string_view::npos) {
    errors->push_back(located_error(source, type_word_start, "expected type name or --json payload"));
    return std::nullopt;
  }
  rest.remove_prefix(type_end);
  lead = rest.find_first_not_of(" \t\r\n");
  if (lead == std::string_view::npos) {
    errors->push_back(located_error(source, type_word_start + type_end,
                                    "expected type name or --json payload"));
    return std::nullopt;
  }
  rest.remove_prefix(lead);
  const auto authoring_offset = type_word_start + type_end + lead;
  AuthoringCommand command;
  if (rest.substr(0, 6) == "--json" && (rest.size() == 6 || std::isspace(rest[6]))) {
    command.json_mode = true;
    rest.remove_prefix(6);
    std::size_t json_start = rest.find_first_not_of(" \t\r\n");
    if (json_start == std::string_view::npos) {
      errors->push_back(located_error(source, authoring_offset + 6,
                                      "expected JSON payload"));
      return std::nullopt;
    }
    rest.remove_prefix(json_start);
    std::string text(rest);
    std::size_t wrapper_offset = 0;
    if (text.size() >= 2 && ((text.front() == '\'' && text.back() == '\'') ||
                             (text.front() == '"' && text.back() == '"'))) {
      wrapper_offset = 1;
      text = text.substr(1, text.size() - 2);
    }
    auto parsed = parse_json(text);
    if (!parsed.errors.empty() || !parsed.value ||
        !std::holds_alternative<ValueObject>(parsed.value->value)) {
      if (!parsed.errors.empty()) {
        errors->push_back(located_error(source, authoring_offset + 6 + json_start +
                                        wrapper_offset + parsed.errors.front().offset,
                                        parsed.errors.front().message));
      } else {
        errors->push_back(located_error(source, authoring_offset + 6 + json_start,
                                        "JSON payload must be an object"));
      }
      return std::nullopt;
    }
    adjust_value_spans(&*parsed.value, source,
                       authoring_offset + 6 + json_start + wrapper_offset);
    command.payload = std::move(*parsed.value);
    const auto& schema = std::get<ValueObject>(command.payload.value);
    auto name = schema.find("name");
    if (name != schema.end() && std::holds_alternative<std::string>(name->second.value)) {
      command.type_name = std::get<std::string>(name->second.value);
      auto name_space = schema.find("namespace");
      if (name_space != schema.end() &&
          std::holds_alternative<std::string>(name_space->second.value) &&
          !std::get<std::string>(name_space->second.value).empty()) {
        command.type_name = std::get<std::string>(name_space->second.value) +
                            "::" + command.type_name;
      }
    }
    return command;
  }

  auto name_end = rest.find_first_of(" \t\r\n");
  command.type_name = std::string(rest.substr(0, name_end));
  if (!is_identifier(command.type_name)) {
    errors->push_back(located_error(source, authoring_offset, "invalid type name"));
    return std::nullopt;
  }
  if (name_end == std::string_view::npos) {
    errors->push_back(located_error(source, authoring_offset + rest.size(),
                                    "expected fields clause"));
    return std::nullopt;
  }
  rest.remove_prefix(name_end);
  lead = rest.find_first_not_of(" \t\r\n");
  if (lead == std::string_view::npos) {
    errors->push_back(located_error(source, authoring_offset + name_end,
                                    "expected fields clause"));
    return std::nullopt;
  }
  rest.remove_prefix(lead);
  const auto clause_offset = authoring_offset + name_end + lead;
  if (rest.substr(0, 6) != "fields" ||
      (rest.size() > 6 && !std::isspace(rest[6]))) {
    errors->push_back(located_error(source, clause_offset, "missing fields clause"));
    return std::nullopt;
  }
  rest.remove_prefix(6);
  const auto fields_offset = clause_offset + 6;
  std::size_t offset = 0;
  while (offset < rest.size()) {
    while (offset < rest.size() &&
           (std::isspace(static_cast<unsigned char>(rest[offset])) || rest[offset] == ',')) ++offset;
    if (offset == rest.size()) break;
    std::size_t end = rest.find(',', offset);
    if (end == std::string_view::npos) end = rest.size();
    auto spec = trim_copy(rest.substr(offset, end - offset));
    auto colon = spec.find(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == spec.size()) {
      errors->push_back(located_error(source, fields_offset + offset,
                                      "invalid field specification"));
      return std::nullopt;
    }
    AuthoringFieldSpec field;
    field.name = trim_copy(std::string_view(spec).substr(0, colon));
    field.type_name = trim_copy(std::string_view(spec).substr(colon + 1));
    if (!field.name.empty() && field.name.back() == '?') {
      field.required = false;
      field.name.pop_back();
    }
    if (!is_identifier(field.name) || !is_identifier(field.type_name)) {
      errors->push_back(located_error(source, fields_offset + offset, "invalid field name"));
      return std::nullopt;
    }
    auto field_location = located_error(source, fields_offset + offset, {});
    field.span = Span{field_location.offset, field_location.line, field_location.column,
                      spec.size()};
    command.fields.push_back(std::move(field));
    offset = end + 1;
  }
  if (command.fields.empty()) {
    errors->push_back(located_error(source, fields_offset, "no fields defined"));
    return std::nullopt;
  }
  command.payload = ValueNode{Value{ValueObject{}}, Span{}};
  return command;
}

void parse_alias_assignment(CommandAst* out, std::string_view input, const Token& name_token) {
  AliasAssignmentCommand command;
  command.keyword = out->name;
  command.persistent = out->name == "var";

  auto rest = trim_copy(tail_after_token(input, name_token));
  if (rest == ".") {
    command.list_aliases = true;
    out->node = std::move(command);
    return;
  }

  auto eq = rest.find('=');
  if (eq == std::string::npos) return;

  command.name = trim_copy(rest.substr(0, eq));
  command.expression = trim_copy(rest.substr(eq + 1));
  if (command.name.empty() || command.expression.empty()) return;

  auto nested_start = command.expression.find_first_not_of(" \t\r\n");
  auto nested_end = nested_start == std::string::npos ? std::string::npos
      : command.expression.find_first_of(" \t\r\n", nested_start);
  auto nested_word = nested_start == std::string::npos ? std::string_view{}
      : std::string_view(command.expression).substr(nested_start,
          (nested_end == std::string::npos ? command.expression.size() : nested_end) - nested_start);
  if (nested_word == "new") {
    std::vector<ParseError> errors;
    command.authoring = parse_new_authoring(command.expression, &errors);
    if (!errors.empty()) out->errors = std::move(errors);
    if (command.authoring) command.expression.clear();
  }

  out->node = std::move(command);
}

} // namespace

CommandAst parse_conch_command(std::string_view input) {
  CommandAst out;
  out.raw_input = std::string(input);
  auto trimmed = trim_copy(input);
  const auto first_end = trimmed.find_first_of(" \t\r\n");
  const auto first_word = std::string_view(trimmed).substr(0, first_end);
  if (first_word == "new") {
    out.name = "new";
    std::vector<ParseError> errors;
    auto authoring = parse_new_authoring(input, &errors);
    out.errors = std::move(errors);
    if (authoring) {
      out.node = ObjectCommand{ObjectCommandKind::New, {}, std::move(authoring)};
    }
    return out;
  }
  std::string_view remainder(trimmed);
  if (first_end != std::string::npos) remainder.remove_prefix(first_end);
  const auto second_start = remainder.find_first_not_of(" \t\r\n");
  const auto second_end = second_start == std::string_view::npos ? std::string_view::npos
      : remainder.find_first_of(" \t\r\n", second_start);
  const auto second_word = second_start == std::string_view::npos ? std::string_view{}
      : remainder.substr(second_start,
          (second_end == std::string_view::npos ? remainder.size() : second_end) - second_start);
  if (first_word == "define" && second_word == "type") {
    out.name = "define";
    std::vector<ParseError> errors;
    auto authoring = parse_define_authoring(input, &errors);
    out.errors = std::move(errors);
    if (authoring) {
      out.node = SchemaCommand{SchemaCommandKind::DefineType,
                               {"type", authoring->type_name},
                               authoring->type_name, std::move(authoring)};
    }
    return out;
  }
  Tokenizer tokenizer;
  auto result = tokenizer.tokenize_loose(input);

  out.errors = std::move(result.errors);

  const Token* name_token = nullptr;
  for (const auto& tok : result.tokens) {
    if (tok.kind == TokenKind::End) break;
    if (out.name.empty()) {
      out.name = tok.text;
      name_token = &tok;
    } else {
      out.args.push_back(tok.text);
    }
  }

  if (!out.errors.empty() || out.name.empty() || name_token == nullptr) return out;

  if (out.name == "caliper") {
    out.node = CaliperCommand{out.args};
    return out;
  }
  if (out.name == "let" || out.name == "var" || out.name == "alias") {
    parse_alias_assignment(&out, input, *name_token);
    return out;
  }
  if (out.name == "ls") {
    out.node = TypesListCommand{ out.args };
    return out;
  }
  if (out.name == "namespace" || out.name == "ns") {
    out.node = NamespaceCommand{ out.name, out.args };
    return out;
  }
  if (out.name == "find" && out.args.size() >= 2 && out.args[0] == "type") {
    out.node = SchemaCommand{ SchemaCommandKind::FindType, out.args, out.args[1], {} };
    return out;
  }
  if (out.name == "show" && out.args.size() == 2 && out.args[0] == "type") {
    out.node = SchemaCommand{ SchemaCommandKind::ShowType, out.args, out.args[1], {} };
    return out;
  }
  if (out.name == "ops" && !out.args.empty()) {
    out.node = SchemaCommand{ SchemaCommandKind::Ops, out.args, out.args[0], {} };
    return out;
  }
  if (out.name == "new") {
    out.errors.push_back(ParseError{"expected type name or --json payload", 0, 1, 1});
    return out;
  }
  if (out.name == "show" && out.args.size() == 1) {
    out.node = ObjectCommand{ ObjectCommandKind::Show, out.args[0], {} };
    return out;
  }
  if (out.name == "edges" && out.args.size() == 1) {
    out.node = ObjectCommand{ ObjectCommandKind::Edges, out.args[0], {} };
    return out;
  }
  if (out.name == "call" && out.args.size() >= 2) {
    CallCommand command;
    command.target = out.args[0];
    command.operation = out.args[1];
    if (out.args.size() > 2) {
      command.args.assign(out.args.begin() + 2, out.args.end());
    }
    out.node = std::move(command);
    return out;
  }
  if (out.name == "start" && out.args.size() == 1) {
    out.node = TaskCommand{ TaskCommandKind::Start, out.args };
    return out;
  }
  if (out.name == "ps" && out.args.empty()) {
    out.node = TaskCommand{ TaskCommandKind::Ps, out.args };
    return out;
  }
  if (out.name == "help" && out.args.empty()) {
    out.node = HelpCommand{};
    return out;
  }
  if (out.name == "kill" && out.args.size() == 1) {
    out.node = TaskCommand{ TaskCommandKind::Kill, out.args };
    return out;
  }
  if (out.name == "task" && !out.args.empty()) {
    out.node = TaskCommand{ TaskCommandKind::Task, out.args };
    return out;
  }
  if (out.name == "io" && !out.args.empty()) {
    out.node = IoCommand{ out.args };
    return out;
  }
  if (out.name == "caps") {
    out.node = CapsCommand{ out.args };
    return out;
  }

  return out;
}

} // namespace iris::parser
