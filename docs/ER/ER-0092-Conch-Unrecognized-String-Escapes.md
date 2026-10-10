---
GitHub-Issue: N/A
AR-Dependencies: AR-0015
ER-Dependencies: ER-0067, ER-0088
---

# ER-0092 — Conch Unrecognized String Escapes

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may mark this ER Verified only after all documented gates pass.

## ER Metadata

- ER ID: ER-0092
- Title: Conch Unrecognized String Escapes
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: AR-0015 requires predictable quoted strings and specifies escaping the quote delimiter and backslash, but it does not define what happens when a backslash precedes another character. The current generic tokenizer consumes the backslash and emits the following character, so inputs such as a quoted Windows-style path lose their separators. A Conch-specific lexer must not accidentally change this behavior while ER-0088 implements the explicitly specified escapes.
- Background / constraints: The System Engineer must select the behavior before implementation. This ER isolates that decision from the structured authoring AST and grammar-conformance work. Do not mark AR-0015 complete until the selected rule is implemented and tested.

## Goals

- Apply one explicit, deterministic rule to backslash followed by a character other than the active quote delimiter or backslash.
- Verify the rule through both public parser entry points and authoring payload parsing where applicable.

## Non-Goals

- Change the accepted quote delimiters or the specified active-quote/backslash escapes.
- Change generic tokenizer behavior for non-Conch parser consumers.

## Scope

- In scope: Conch quoted-string decoding for unrecognized escapes, parser/API diagnostics if the selected rule rejects input, and regression tests for the selected behavior.
- Out of scope: other grammar productions, authoring AST structure, and unrelated tokenizer consumers.

## Requirements

- Functional: use the System Engineer's recorded ruling: retain current behavior by dropping the backslash, reject the unrecognized escape, or preserve both characters.
- Functional: test backslash followed by an ordinary character (including `q`), a character with path significance such as `t`, and both quote styles where the escape can occur.
- Functional: `parse_conch_command` and `parse_conch_grammar` return matching decoded values or matching error message, byte offset, line, and column according to the selected rule.
- Functional: if the rule changes authoring payload values, scripted shell tests verify the resulting persisted field value; otherwise assert authoring behavior remains consistent with the public parser result.
- Non-functional: keep behavior deterministic and scoped to Conch strings; generic tokenizer behavior remains unchanged.

## Proposed Approach

- Summary: add explicit fixtures for unrecognized escape sequences and assert the selected semantics at the public parser boundary. Add one authoring round-trip test if the selected behavior affects a `new` payload. Record the ruling in this ER before implementation.
- Alternatives considered: embedding the decision in ER-0088 was rejected because it would couple a separately ruled compatibility behavior to the structured AST change and could silently change existing payload values.

## Acceptance Criteria

- The System Engineer's selected behavior is recorded in this ER before implementation begins.
- Tests cover `\\q`, `\\t`, and both quote forms wherever applicable, with exact decoded values or exact diagnostics.
- Both public parser entry points agree on the selected behavior and source locations for rejected input.
- A shell authoring test proves the selected decoded value is persisted when the rule affects authoring payloads.
- Existing Conch parser, grammar API, authoring, and full repository test suites pass.

## Risks / Open Questions

- Risk: changing escape handling can alter existing string values, especially paths and other strings containing backslashes.
- Question for System Engineer: retain current behavior by dropping the backslash, reject unrecognized escapes, or preserve both characters?

## Dependencies

- Dependency 1: ER-0067 Reusable Conch Grammar API.
- Dependency 2: ER-0088 Conch Structured Authoring AST.

## Implementation Notes

- Do not infer the selected rule from current implementation behavior; require the recorded System Engineer ruling.
- Do not modify the generic `Tokenizer` behavior for non-Conch consumers.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_conch_parser test_conch_grammar_api test_conch_parser_regression test_conch_authoring'`
  - `make check`
  - `git diff --check`
- Manual checks: Not required; parser and scripted authoring tests make the decoded behavior observable.

### Live Pass (Human)

- Required: No
- If no, reason: The selected string-decoding behavior and authoring persistence are fully observable in automated parser and scripted shell tests.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
