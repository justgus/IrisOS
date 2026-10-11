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
- Status: Complete
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: AR-0015 requires predictable quoted strings and specifies escaping the quote delimiter and backslash, but it does not define what happens when a backslash precedes another character. The current generic tokenizer consumes the backslash and emits the following character, so inputs such as a quoted Windows-style path lose their separators. A Conch-specific lexer must not accidentally change this behavior while ER-0088 implements the explicitly specified escapes.
- Background / constraints: The System Engineer selected preservation of both characters for unknown escapes. This ER isolates that behavior from the structured authoring AST and grammar-conformance work. Do not mark AR-0015 complete until the rule is implemented and tested.

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

- Functional: preserve both the backslash and following character when it is not the active quote delimiter or a backslash.
- Functional: test backslash followed by an ordinary character (including `q`), a character with path significance such as `t`, and both quote styles where the escape can occur.
- Functional: `parse_conch_command` and `parse_conch_grammar` return matching decoded values or matching error message, byte offset, line, and column according to the selected rule.
- Functional: if the rule changes authoring payload values, scripted shell tests verify the resulting persisted field value; otherwise assert authoring behavior remains consistent with the public parser result.
- Non-functional: keep behavior deterministic and scoped to Conch strings; generic tokenizer behavior remains unchanged.

## Proposed Approach

- Summary: preserve the backslash for unrecognized escapes in both the Conch tokenizer and inline authoring parser. Test both public parser entry points and verify decoded values in persisted authoring payloads.
- Alternatives considered: embedding the decision in ER-0088 was rejected because it would couple a separately ruled compatibility behavior to the structured AST change and could silently change existing payload values.

## Acceptance Criteria

- The System Engineer's selected behavior is recorded here: preserve both characters for unknown escapes such as `\\q` and `\\t`; escape only the active quote delimiter and backslash.
- Tests cover `\\q`, `\\t`, and both quote forms wherever applicable, with exact decoded values or exact diagnostics.
- Both public parser entry points agree on the selected behavior and source locations for rejected input.
- A shell authoring test proves the selected decoded value is persisted when the rule affects authoring payloads.
- Existing Conch parser, grammar API, authoring, and full repository test suites pass.

## Risks / Open Questions

- Risk: callers that relied on the previous backslash-dropping behavior will now receive the literal backslash. This is the selected compatibility correction for Conch strings.

## Dependencies

- Dependency 1: ER-0067 Reusable Conch Grammar API.
- Dependency 2: ER-0088 Conch Structured Authoring AST.

## Implementation Notes

- System Engineer ruling (2026-10-10): preserve both characters for unknown escapes such as `\\q` and `\\t`; escape only the active quote delimiter and backslash.
- Do not modify the generic `Tokenizer` behavior for non-Conch consumers.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh` (passed)
  - `./configure` (passed)
  - `make -j2 CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` (passed)
  - `make -C tests check TESTS='test_conch_parser test_conch_grammar_api test_conch_parser_regression test_conch_authoring test_parser_core' CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` (passed 5/5)
  - `DYLD_LIBRARY_PATH="$PWD/src/.libs" TMPDIR=/private/tmp make -j1 check CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` (passed 30/30)
  - `git diff --check` (passed)
- Manual checks: Not required; parser and scripted authoring tests make the decoded behavior observable.

### Independent Live Pass

- Required: No
- If no, reason: The selected string-decoding behavior and authoring persistence are fully observable in automated parser and scripted shell tests.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable


## Implementation Review

- Independent review: Pass. The read-only reviewer checked each acceptance criterion, counterexamples for both quote forms and all required unknown escapes, parity through both public parser APIs, persisted authoring values, and unchanged generic Tokenizer behavior; no findings remain.
- Fresh independent validation: focused command passed 5/5, including `test_parser_core`; `git diff --check` passed.
- CI / merge / final verification: Pending the authorized AR-0015 delivery PR and final batch reconciliation.
