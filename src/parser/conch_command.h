#pragma once

#include "parser/tokenizer.h"

#include <string>
#include <variant>
#include <vector>

namespace iris::parser {

struct AliasAssignmentCommand {
  std::string keyword;
  bool persistent{false};
  bool list_aliases{false};
  std::string name;
  std::string expression;
};

struct TypesListCommand {
  std::vector<std::string> args;
};

struct NamespaceCommand {
  std::string keyword;
  std::vector<std::string> args;
};

enum class SchemaCommandKind {
  DefineType,
  FindType,
  ShowType,
  Ops
};

struct SchemaCommand {
  SchemaCommandKind kind{SchemaCommandKind::FindType};
  std::vector<std::string> args;
  std::vector<std::string> tokens;
  std::string type_name;
};

enum class ObjectCommandKind {
  New,
  Show,
  Edges
};

struct ObjectCommand {
  ObjectCommandKind kind{ObjectCommandKind::Show};
  std::string expression;
  std::string target;
};

struct CallCommand {
  std::string target;
  std::string operation;
  std::vector<std::string> args;
};

enum class TaskCommandKind {
  Start,
  Ps,
  Kill,
  Task
};

struct TaskCommand {
  TaskCommandKind kind{TaskCommandKind::Task};
  std::vector<std::string> args;
};

struct IoCommand {
  std::vector<std::string> args;
};

struct CapsCommand {
  std::vector<std::string> args;
};

using CommandNode = std::variant<std::monostate,
                                 AliasAssignmentCommand,
                                 TypesListCommand,
                                 NamespaceCommand,
                                 SchemaCommand,
                                 ObjectCommand,
                                 CallCommand,
                                 TaskCommand,
                                 IoCommand,
                                 CapsCommand>;

struct CommandAst {
  std::string raw_input;
  std::string name;
  std::vector<std::string> args;
  std::vector<ParseError> errors;
  CommandNode node{};

  template <typename T>
  const T* get_if() const {
    return std::get_if<T>(&node);
  }
};

CommandAst parse_conch_command(std::string_view input);

} // namespace iris::parser
