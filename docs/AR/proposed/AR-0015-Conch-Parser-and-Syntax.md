# AR-0015 — Conch Parser and Syntax (Proposal)

- Status: Proposed
- Date: 2026-02-21
- Owners: Mike

## Context

Conch currently tokenizes input by splitting on whitespace. This makes quoted values with spaces
impossible in inline forms (for example `new Type value:="hello world"`), and forces JSON usage
for any complex payloads. The limitation also creates inconsistent behavior across commands and
blocks a cleaner authoring experience.

Conch is expected to grow into a primary authoring surface. We need a reusable parser that:

- Handles quotes and escapes predictably.
- Allows flexible whitespace around operators.
- Produces a structured command AST that can be validated independently of input syntax.
- Can be reused by other components (authoring tools, scripted execution, tests).

## Recommendation

Define a standalone Conch parser class that converts a single input line into a typed command AST.
The parser is a dedicated component (not embedded in command handlers), with a small, explicit
grammar and a deterministic tokenizer.

### Goals

- Preserve existing commands and semantics.
- Accept both compact and whitespace-tolerant syntax.
- Keep JSON support as an explicit mode.
- Provide stable AST nodes for validation and execution.

### Non-Goals (v1)

- Full shell scripting (pipes, conditionals, loops).
- Multi-line parsing or heredocs.
- Dynamic evaluation beyond literal parsing.

## Proposed Parser Surface

- Class: `ConchParser`
- Input: `std::string_view line`
- Output: `Result<CommandAst>` (error includes offset + message)
- Tokenizer: recognizes identifiers, string literals, numbers, operators, delimiters.

### Command AST (Sketch)

- `CommandAst { kind, args, kv_pairs, raw_json }`
- `kind`: `ls`, `show`, `define_type`, `new`, `call`, `start`, `alias`, etc.
- `kv_pairs`: ordered `field := value` pairs
- `raw_json`: retained for `--json` commands

## Syntax and Grammar (Broad Strokes)

### Tokens

- Identifiers: `[A-Za-z_][A-Za-z0-9_:.]*`
- Numbers: `-?[0-9]+`
- Strings: `"..."` or `'...'` with `\\` escapes for quotes and backslash
- Operators: `:=`, `=`, `--`

### Whitespace

- Ignored between tokens.
- Allowed around operators (`value := "x"`, `value:= "x"`, `value := "x"`).

### EBNF (Minimal)

```
line         := command
command      := ls_cmd | show_cmd | define_cmd | new_cmd | call_cmd | start_cmd | alias_cmd | help_cmd

ls_cmd       := "ls" [ "--regex" ] [ "--namespaces" ] [ pattern ]
show_cmd     := "show" ( "type" type_name | object_id )
define_cmd   := "define" "type" type_name ( "fields" field_list | "--json" json )
new_cmd      := "new" type_name kv_list | "new" "--json" json
call_cmd     := "call" object_id op_name [ args ]
start_cmd    := "start" object_id
alias_cmd    := ("let" | "var" | "alias") name "=" expr
help_cmd     := "help"

kv_list      := kv_pair { kv_pair }
kv_pair      := name ":=" value
value        := string | number | "true" | "false" | identifier
```

## Compatibility Notes

- Existing whitespace-sensitive inputs remain valid.
- JSON mode remains available for complex payloads.
- The parser returns a structured AST so validation rules can be added without changing syntax.

## Next Steps

- Implement `ConchParser` with unit tests for tokenization and parsing.
- Migrate `conch.cc` to use the parser output rather than raw token vectors.
- Add validation against Refract definitions for `new` and `define type` inputs.
