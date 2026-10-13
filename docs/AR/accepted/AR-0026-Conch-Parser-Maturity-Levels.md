---
GitHub-Issue: #294
ER-Dependencies: ER-0056, ER-0067, ER-0068, ER-0069, ER-0088, ER-0091, ER-0092
---

# AR-0026 — Conch Parser Maturity Levels

- Status: Implemented
- Date: 2026-03-14
- Owners: Mike

## Context

AR-0015 established the parser direction correctly, but the implementation has only completed the
first useful level of that design: quote-aware tokenization with a basic command AST. The accepted
architecture and the verified parser ERs currently read as though the parser has already reached a
richer typed-AST and parser-driven command-dispatch state.

## Recommendation

Define parser maturity levels for Conch.

### Level 1: Quote-Aware Command Parsing

- Deterministic tokenization
- Basic command AST with command name and ordered arguments
- Location-aware parse errors

### Level 2: Typed Command Family AST

- Distinct AST structures for command families such as:
  - schema and type commands
  - object commands
  - operation calls
  - task commands
  - IO commands
  - namespace commands

### Level 3: Parser-Driven Validation And Dispatch

- Command validation operates on typed AST
- Command handlers consume typed AST rather than ad-hoc token reconstruction
- Shell parsing logic is decomposed from `conch.cc`

### Level 4: Reusable Command Grammar Surface

- Parser is reusable for userland tools, batch execution, and future scripting surfaces

## Goals

- Document the current parser as a legitimate but partial architecture stage.
- Provide a concrete target for the next Conch refactor wave.
- Reduce ambiguity around what ER-0023 actually completed.

## Non-Goals (v1)

- Full shell scripting
- Arbitrary expression evaluation
- Backward-incompatible grammar redesign

## Relationship To Existing ARs

- Refines [AR-0015-Conch-Parser-and-Syntax.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0015-Conch-Parser-and-Syntax.md)

## Next Steps

- Levels 1 through 4 are implemented for the documented command-family scope. Full scripting and
  arbitrary expressions remain outside v1.

## Implementation Progress

- `src/parser/tokenizer.cc` and `conch_command.{h,cc}` implement quote-aware tokenization,
  location-aware diagnostics, typed command families, and structured authoring AST data.
- The major command-family handlers consume that typed AST in `src/conch_shell/conch.cc`.
  ER-0088 removes authoring payload reconstruction; ER-0091/0092 cover grammar and escape behavior.
- ER-0067 provides `parse_conch_grammar`, ER-0068 provides noninteractive batch execution, and
  ER-0069 compares reusable/shell parser behavior through shared regression fixtures.
- All listed ER dependencies are Verified. Fresh `TMPDIR=/private/tmp make check` passed 31/31
  outside the sandbox on 2026-10-13, including parser, grammar, regression, and shell suites.
