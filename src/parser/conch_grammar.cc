#include "parser/conch_grammar.h"

namespace iris::parser {

ConchParseResult parse_conch_grammar(std::string_view input) {
  ConchParseResult result;
  result.ast = parse_conch_command(input);
  return result;
}

std::string command_node_kind(const CommandAst& ast) {
  if (ast.get_if<AliasAssignmentCommand>()) return "AliasAssignment";
  if (ast.get_if<TypesListCommand>()) return "TypesList";
  if (ast.get_if<NamespaceCommand>()) return "Namespace";
  if (ast.get_if<SchemaCommand>()) return "Schema";
  if (ast.get_if<ObjectCommand>()) return "Object";
  if (ast.get_if<CallCommand>()) return "Call";
  if (ast.get_if<TaskCommand>()) return "Task";
  if (ast.get_if<IoCommand>()) return "Io";
  if (ast.get_if<CapsCommand>()) return "Caps";
  return "Unknown";
}

NormalizedCommand normalize_command(const CommandAst& ast) {
  NormalizedCommand out;
  out.name = ast.name;
  out.node_kind = command_node_kind(ast);
  out.args = ast.args;
  return out;
}

} // namespace iris::parser
