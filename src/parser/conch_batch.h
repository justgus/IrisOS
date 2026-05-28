#pragma once

#include "parser/conch_grammar.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iris::parser {

enum class BatchCommandStatus {
  Executed,
  ParseError,
  Unsupported,
  ExecutionError
};

struct BatchCommandResult {
  std::size_t command_index{0};
  std::size_t line{0};
  std::string input;
  BatchCommandStatus status{BatchCommandStatus::Executed};
  NormalizedCommand command{};
  std::vector<ParseError> errors{};
  std::string message;
};

struct BatchExecutionResult {
  bool ok{true};
  std::vector<BatchCommandResult> completed{};
  std::optional<BatchCommandResult> failure{};
};

using BatchCommandExecutor = std::function<std::optional<std::string>(const CommandAst&)>;

BatchExecutionResult execute_conch_batch(std::string_view input,
                                         const BatchCommandExecutor& executor);
BatchExecutionResult execute_conch_batch(std::string_view input);

} // namespace iris::parser
