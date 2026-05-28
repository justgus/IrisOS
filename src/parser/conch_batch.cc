#include "parser/conch_batch.h"

#include <cctype>

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

bool is_supported_batch_command(const CommandAst& ast) {
  return ast.get_if<AliasAssignmentCommand>() != nullptr
      || ast.get_if<TypesListCommand>() != nullptr
      || ast.get_if<NamespaceCommand>() != nullptr
      || ast.get_if<SchemaCommand>() != nullptr
      || ast.get_if<ObjectCommand>() != nullptr
      || ast.get_if<CallCommand>() != nullptr
      || ast.get_if<TaskCommand>() != nullptr
      || ast.get_if<IoCommand>() != nullptr
      || ast.get_if<CapsCommand>() != nullptr;
}

void push_command(BatchExecutionResult& out, BatchCommandResult result) {
  if (result.status == BatchCommandStatus::Executed) {
    out.completed.push_back(std::move(result));
    return;
  }

  out.ok = false;
  out.failure = std::move(result);
}

} // namespace

BatchExecutionResult execute_conch_batch(std::string_view input,
                                         const BatchCommandExecutor& executor) {
  BatchExecutionResult out;
  std::size_t command_index = 0;
  std::size_t line_number = 1;
  std::size_t offset = 0;

  while (offset <= input.size()) {
    const auto line_start = offset;
    auto line_end = input.find('\n', offset);
    if (line_end == std::string_view::npos) line_end = input.size();

    auto raw_line = input.substr(line_start, line_end - line_start);
    auto line = trim_copy(raw_line);
    if (!line.empty() && line.rfind("//", 0) != 0) {
      BatchCommandResult result;
      result.command_index = command_index;
      result.line = line_number;
      result.input = line;

      auto parsed = parse_conch_grammar(line);
      if (!parsed.ast.errors.empty()) {
        result.status = BatchCommandStatus::ParseError;
        result.errors = parsed.ast.errors;
        result.message = parsed.ast.errors.front().message;
        push_command(out, std::move(result));
        return out;
      }
      if (parsed.ast.name.empty() || !is_supported_batch_command(parsed.ast)) {
        result.status = BatchCommandStatus::Unsupported;
        result.message = parsed.ast.name.empty() ? "empty command" : "unsupported command";
        push_command(out, std::move(result));
        return out;
      }

      result.command = normalize_command(parsed.ast);
      if (executor) {
        auto execution_error = executor(parsed.ast);
        if (execution_error.has_value()) {
          result.status = BatchCommandStatus::ExecutionError;
          result.message = execution_error.value();
          push_command(out, std::move(result));
          return out;
        }
      }

      result.status = BatchCommandStatus::Executed;
      push_command(out, std::move(result));
      ++command_index;
    }

    if (line_end == input.size()) break;
    offset = line_end + 1;
    ++line_number;
  }

  return out;
}

BatchExecutionResult execute_conch_batch(std::string_view input) {
  return execute_conch_batch(input, {});
}

} // namespace iris::parser
