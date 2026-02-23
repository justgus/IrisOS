#include "parser/conch_command.h"

namespace iris::parser {

CommandAst parse_conch_command(std::string_view input) {
  Tokenizer tokenizer;
  auto result = tokenizer.tokenize_loose(input);

  CommandAst out;
  out.errors = std::move(result.errors);

  for (const auto& tok : result.tokens) {
    if (tok.kind == TokenKind::End) break;
    if (out.name.empty()) {
      out.name = tok.text;
    } else {
      out.args.push_back(tok.text);
    }
  }

  return out;
}

} // namespace iris::parser
