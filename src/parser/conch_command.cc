#include "parser/conch_command.h"

#include <cctype>
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

std::string_view tail_after_token(std::string_view input, const Token& token) {
  auto start = token.span.offset + token.span.length;
  if (start >= input.size()) return {};
  return input.substr(start);
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

  out->node = std::move(command);
}

} // namespace

CommandAst parse_conch_command(std::string_view input) {
  Tokenizer tokenizer;
  auto result = tokenizer.tokenize_loose(input);

  CommandAst out;
  out.raw_input = std::string(input);
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
  if (out.name == "define" && out.args.size() >= 2 && out.args[0] == "type") {
    std::vector<std::string> tokens;
    tokens.reserve(out.args.size() + 1);
    tokens.push_back(out.name);
    tokens.insert(tokens.end(), out.args.begin(), out.args.end());
    out.node = SchemaCommand{ SchemaCommandKind::DefineType, out.args, std::move(tokens), {} };
    return out;
  }
  if (out.name == "find" && out.args.size() >= 2 && out.args[0] == "type") {
    out.node = SchemaCommand{ SchemaCommandKind::FindType, out.args, {}, out.args[1] };
    return out;
  }
  if (out.name == "show" && out.args.size() == 2 && out.args[0] == "type") {
    out.node = SchemaCommand{ SchemaCommandKind::ShowType, out.args, {}, out.args[1] };
    return out;
  }
  if (out.name == "ops" && !out.args.empty()) {
    out.node = SchemaCommand{ SchemaCommandKind::Ops, out.args, {}, out.args[0] };
    return out;
  }
  if (out.name == "new") {
    out.node = ObjectCommand{ ObjectCommandKind::New, out.raw_input, {} };
    return out;
  }
  if (out.name == "show" && out.args.size() == 1) {
    out.node = ObjectCommand{ ObjectCommandKind::Show, {}, out.args[0] };
    return out;
  }
  if (out.name == "edges" && out.args.size() == 1) {
    out.node = ObjectCommand{ ObjectCommandKind::Edges, {}, out.args[0] };
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
