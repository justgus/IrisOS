#pragma once

#include "parser/types.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iris::parser {

struct XmlAttribute {
  std::string name;
  std::string value;
  Span span;
};

struct XmlNode {
  enum class Kind {
    Element,
    Text
  };

  Kind kind{Kind::Element};
  std::string name;
  std::string text;
  std::vector<XmlAttribute> attributes;
  std::vector<XmlNode> children;
  Span span;
};

struct XmlParseResult {
  std::optional<XmlNode> root;
  std::vector<ParseError> errors;
};

XmlParseResult parse_xml(std::string_view input);

} // namespace iris::parser
