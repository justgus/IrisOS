---
GitHub-Issue: N/A
ER-Dependencies: ER-0056
---

# AR-0026 — Conch Parser Maturity Levels

- Status: Proposed
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

- Draft ER-0056 to implement Level 2 and begin Level 3.
