#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace iris::parser {

struct Span {
  std::size_t offset{0};
  std::size_t line{1};
  std::size_t column{1};
  std::size_t length{0};
};

struct ParseError {
  std::string message;
  std::size_t offset{0};
  std::size_t line{1};
  std::size_t column{1};
};

struct ValueNode;

using ValueArray = std::vector<ValueNode>;
using ValueObject = std::map<std::string, ValueNode>;
using Value = std::variant<std::monostate, bool, double, std::string, ValueArray, ValueObject>;

struct ValueNode {
  Value value;
  Span span;
};

} // namespace iris::parser
