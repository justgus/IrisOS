---
GitHub-Issue: #343
AR-Dependencies: AR-0015
ER-Dependencies: ER-0067, ER-0069, ER-0088
---

# ER-0091 — Conch Grammar Conformance Fixtures

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may mark this ER Verified only after all documented gates pass.

## ER Metadata

- ER ID: ER-0091
- Title: Conch Grammar Conformance Fixtures
- Status: Approved
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: AR-0015 requires a reusable parser with stable typed ASTs and compatibility with its documented command grammar, but the shared parser regression fixture currently covers only four accepted forms and one malformed string. Direct unit tests exercise additional command nodes but do not provide fixture-based parity coverage for every documented production.
- Background / constraints: ER-0067 exposes the reusable grammar API and ER-0069 provides shell/API regression infrastructure. This ER adds conformance evidence for the accepted grammar without changing command semantics.

## Goals

- Represent every documented AR-0015 command production in the shared parser regression fixture.
- Compare normalized ASTs, errors, and source locations through the shell parser and reusable grammar API.

## Non-Goals

- Expand the grammar beyond forms accepted by AR-0015.
- Change parser behavior except to correct a demonstrated mismatch with the accepted grammar.

## Scope

- In scope: `tests/fixtures/parser/conch_commands.tsv`, regression and direct parser/tokenizer assertions, and narrowly scoped parser fixes if fixtures expose a mismatch.
- Out of scope: authoring payload structure and numeric representation covered by ER-0088; new command families or shell scripting.

## Requirements

- Functional: fixture coverage includes all eight combinations of the optional `ls --regex`, `--namespaces`, and pattern fields; `show type`, `show` object, inline and JSON `define type`, inline and JSON `new`, `call` with and without arguments, `start`, `let`/`var`/`alias` assignment, and `help`, using the forms documented in AR-0015.
- Functional: accepted fixtures compare normalized command kind/arguments and relevant typed AST fields through both `parse_conch_command` and `parse_conch_grammar`; inline/JSON authoring fixtures also assert the structured type name, payload fields, value kinds, and values defined by ER-0088.
- Functional: malformed fixtures cover unterminated quotes, malformed operator/value structure, and invalid command shape; both APIs return matching stable message, byte offset, line, and column.
- Functional: explicit token-level fixtures/assertions cover identifiers matching `[A-Za-z_][A-Za-z0-9_:.]*` (including namespace colons and dotted names), signed integers matching `-?[0-9]+`, both single- and double-quoted strings, escaping the active quote delimiter and backslash in each quote form, the `:=`, `=`, and `--` operators, and JSON punctuation tokens `{`, `}`, `[`, `]`, `,`, and `:`. The other quote character is literal inside a string. Invalid identifier starts or hyphens must not be accepted as part of a single identifier token. Unrecognized backslash escapes are excluded here and owned by ER-0092. A token-level assertion does not replace command-AST conformance fixtures.
- Non-functional: fixture results remain deterministic and require no shell session or external service.

## Proposed Approach

- Summary: expand the fixture record format with explicit expected structured-node data and error byte-offset/line/column fields, and update regression assertions to compare those values. Sequence authoring fixtures after ER-0088 so the expected AST contract matches the structured authoring AST. Keep AST normalization stable and avoid encoding unescaped tabs/newlines in the fixture format.
- Alternatives considered: relying only on individual parser unit tests was rejected because it does not prove shell/reusable API parity across the documented grammar.

## Acceptance Criteria

- A fixture exists for each production and subform listed in the functional requirement, with a normalized expected node kind, arguments, and expected structured-node fields; authoring entries include expected payload/value-kind data from ER-0088.
- Token-level tests explicitly cover the AR-0015 token classes and operators, including both string quote forms and supported escapes.
- Token fixtures assert namespace-qualified and dotted identifiers remain single identifier tokens, while invalid starts and hyphens do not extend that token class.
- Both parser entry points produce the expected normalized AST for every accepted fixture.
- Every malformed fixture records its expected error message, byte offset, line, and column; the test asserts those exact values from each entry point and asserts the entry points agree.
- The fixture suite exercises quoted strings, supported escapes, and whitespace around `:=` where applicable; numeric boundary preservation is owned by ER-0088.
- Existing parser, grammar API, regression, authoring, and full repository test suites pass.

## Risks / Open Questions

- Risk: command aliases or parser normalization may intentionally differ; fixtures should assert stable semantic AST fields rather than incidental formatting.
- Limitation: `parse_conch_grammar` currently delegates to `parse_conch_command`; parity assertions document the public API contract but do not constitute independent parser implementations.
- Deferred behavior decision: The accepted grammar does not define unrecognized backslash escapes. ER-0092 records the System Engineer ruling request and owns the corresponding fixtures.

## Dependencies

- Dependency 1: ER-0067 Reusable Conch Grammar API.
- Dependency 2: ER-0069 Shared Parser Regression Harness.
- Dependency 3: ER-0088 Conch Structured Authoring AST; authoring-form fixtures are implemented after its AST contract is merged.

## Implementation Notes

- Keep the fixture schema simple and readable. If tabs or newlines are required in a command value, add a narrowly scoped encoding rule with a round-trip assertion.
- Do not edit ER-0067/0069 status; this is follow-up coverage for the accepted grammar.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_conch_parser test_conch_grammar_api test_conch_parser_regression test_conch_authoring'`
  - `make check`
  - `git diff --check`
- Manual checks: Not required; fixture-based parser/API parity is fully observable in automated tests.

### Live Pass (Human)

- Required: No
- If no, reason: Grammar conformance and parser API parity are fully testable in the local parser and scripted shell harnesses.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
