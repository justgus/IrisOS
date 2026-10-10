---
GitHub-Issue: #341
AR-Dependencies: AR-0015
ER-Dependencies: ER-0056, ER-0067
---

# ER-0088 — Conch Structured Authoring AST

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may mark this ER Verified only after all documented gates pass.

## ER Metadata

- ER ID: ER-0088
- Title: Conch Structured Authoring AST
- Status: Approved
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: `new` and `define type` command AST nodes still preserve token vectors or raw command text. Shell execution separately reparses key/value payload text, so command shape validation is coupled to execution and the parser cannot fully provide the stable structured AST required by AR-0015.
- Background / constraints: AR-0015 requires an AST that can be validated independently of input syntax and preserves the explicit JSON mode. ER-0056 introduced typed AST nodes and ER-0067 exposed the parser as a reusable API; this ER closes the remaining authoring-command representation gap without changing accepted command semantics.

## Goals

- Represent `new` and `define type` command shape, type name, and payload in typed AST data rather than opaque command text or token vectors.
- Have the shell consume the structured AST for these command families instead of reparsing their command text.
- Preserve inline `field:=value` forms, quoted values, flexible whitespace around `:=`, and explicit `--json` behavior.
- Preserve the currently accepted `let name=new ...` alias form while moving its payload parsing into the shared AST.

## Non-Goals

- Add shell scripting, expressions, or new command families.
- Change Refract field validation or object persistence semantics.

## Scope

- In scope: Conch-specific lexical handling for the AR-0015 tokens, parser AST definitions, parsing and deterministic diagnostics for inline and JSON authoring forms, shell integration, and parser/shell regression tests.
- Out of scope: other Conch command families and changes to the accepted JSON data model.

## Requirements

- Functional: `new` and `define type` inputs parse into typed nodes containing the command's type name and structured field/specification values.
- Functional: accepted whitespace, both single- and double-quoted strings, quote/backslash escapes, scalar values, and JSON payloads follow the AR-0015 grammar for both `new` and `define type`.
- Functional: inline signed 64-bit integer values retain exact integer representation; converting them through binary64 must not lose precision.
- Functional: JSON numeric values preserve their existing nlohmann JSON integer, unsigned integer, and floating-point distinctions and exact integer values; the structured AST must not route JSON integers through binary64.
- Functional: malformed command shape or payload reports a deterministic parse/validation error with the relevant location where available.
- Non-functional: shell execution and non-shell consumers use the same parser result as their command-shape source.

## Proposed Approach

- Summary: give Conch parsing lexical handling for the identifiers, numbers, strings, delimiters, and multi-character operators in AR-0015, without changing the generic tokenizer behavior for other parser users. Extend the typed command AST with explicit authoring payload/value structures, including an authoring number representation that preserves number kind and source lexeme until conversion to the existing JSON model. Parse inline key/value and JSON modes into these nodes and update shell execution to consume them. Add direct parser assertions and end-to-end scripted Conch regression cases for both forms and malformed inputs.
- Alternatives considered: retaining raw expression text and duplicating parsing in shell execution was rejected because it leaves the AR-0015 validation boundary unfulfilled.

## Acceptance Criteria

- Parser tests assert `new` produces a typed node with the type name and structured payload for unquoted and quoted scalar fields, including both quote forms, whitespace around `:=`, escaped quotes/backslashes, and signed 64-bit integer boundaries.
- Provisional compatibility tests assert that an authoring string containing `\q` decodes to `q` and one containing `\t` decodes to `t`, matching the current tokenizer's output. These assertions prevent an accidental behavior change during this ER; ER-0092 must replace or confirm them after the System Engineer's ruling.
- A regression case for an authoring string containing `C:\temp` asserts that it decodes to `C:temp` under the current generic-tokenizer behavior; any later behavior change belongs only to ER-0092 after the System Engineer's ruling.
- Conch parsing recognizes identifiers matching `[A-Za-z_][A-Za-z0-9_:.]*`, signed integers matching `-?[0-9]+`, single- and double-quoted strings, the `:=`, `=`, and `--` operators, and the delimiters used by JSON mode, with source spans. In each quote form, the active quote delimiter and backslash can be escaped; the other quote character is literal. For this ER, a backslash before any other character preserves the current tokenizer output; ER-0092 owns the final compatibility rule and its acceptance tests. Other parser consumers retain their existing generic tokenizer behavior.
- Parser tests assert inline `define type` produces its structured type specification, including multiple fields, optional fields, and malformed field/value diagnostics.
- Parser tests assert `define type --json` and `new --json` retain the expected structured values, preserve signed/unsigned integer values above 2^53 exactly, preserve floating-point values, and reject malformed JSON with stable diagnostics.
- The alias AST for `let name=new ...` carries the nested `new` command and structured payload (or an equivalent typed representation); parser tests compare its payload with the direct `new` AST, and shell regression tests assert both commands create the same object without reparsing a raw expression.
- Malformed inline `new` and `define type` tests assert deterministic errors and source locations; the same invalid inputs must produce matching line/column locations through the shell parser and reusable grammar API. Where a downstream schema/type validation error has no command-source location, the test asserts the stable diagnostic without requiring a fabricated location.
- Shell integration tests demonstrate inline and JSON forms create the same expected Refract definitions/objects as before.
- Shell execution no longer reparses raw command text to recover the `new`/`define type` command shape or payload.
- Existing parser, authoring, and full repository test suites pass.

## Risks / Open Questions

- Risk: parsing values twice or subtly changing escape semantics could break existing authoring commands.
- Deferred behavior decision: ER-0092 owns whether a backslash before a non-quote, non-backslash character retains current tokenizer behavior, is rejected, or preserves both characters. This ER must not change that behavior. `ValueNode` currently stores numbers as `double`, so do not use it as the lossless numeric representation for these authoring payloads. Retain numeric kind and source lexeme in the authoring AST, then materialize the same integer/unsigned/floating alternatives used by the existing JSON model.

## Dependencies

- Dependency 1: ER-0056 Conch Typed AST and Shell Decomposition.
- Dependency 2: ER-0067 Reusable Conch Grammar API.

## Implementation Notes

- Keep the change limited to the already accepted authoring forms; do not extend `value` syntax beyond current behavior.
- Avoid parsing the same payload independently in parser and shell paths.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_conch_parser test_conch_grammar_api test_conch_parser_regression test_conch_authoring'`
  - `make check`
  - `git diff --check`
- Manual checks: Not required; scripted shell integration tests cover both supported authoring modes and error behavior.

### Live Pass (Human)

- Required: No
- If no, reason: The parser representation and shell behavior are fully observable in automated parser and subprocess integration tests; no target-specific environment or interaction is required.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
