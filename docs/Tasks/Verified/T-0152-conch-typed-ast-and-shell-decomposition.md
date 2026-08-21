---
Legacy-ID: ER-0056
GitHub-Issue: #263
Source-Path: docs/ER/ER-0056-Conch-Typed-AST-and-Shell-Decomposition.md
---

# T-0152 — Conch Typed AST and Shell Decomposition

## Task Metadata

- Task ID: T-0152
- Legacy ID: ER-0056
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #263
- AR Dependencies: -
- Date Requested: 2026-03-14
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0056-Conch-Typed-AST-and-Shell-Decomposition.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #263
---

# ER-0056 — Conch Typed AST and Shell Decomposition

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0056
- Title: Conch Typed AST and Shell Decomposition
- Status: Verified
- Date: 2026-03-14
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Conch parsing is still mostly a quote-aware token pass feeding a large monolithic command loop in `conch.cc`.
- Background / constraints: AR-0015 and AR-0026 define a path from basic parsing to typed AST and parser-driven command handling. The existing shell behavior must remain stable while the implementation is decomposed.

## Goals

- Introduce a typed AST for major built-in command families.
- Move validation and command-shape parsing out of the monolithic shell loop.
- Reduce ad-hoc token reconstruction and command-specific parsing inside `conch.cc`.

## Non-Goals

- Full shell scripting.
- Grammar redesign that breaks existing commands without explicit approval.
- Rewriting every command handler in one change.

## Scope

- In scope: typed AST for schema, object, call, task, IO, and namespace command families.
- In scope: parser updates and command-handler decomposition needed to consume the new AST.
- In scope: regression coverage for spacing, quoting, key/value pairs, and aliases.
- Out of scope: user-defined language features or general scripting semantics.

## Requirements

- Functional: major built-in command families parse into typed AST nodes.
- Functional: command handlers consume the typed AST for those families rather than reconstructing tokens ad hoc.
- Functional: existing command behavior remains backward compatible unless explicitly documented otherwise.
- Non-functional: parsing and command dispatch become easier to test independently of the interactive loop.

## Proposed Approach

- Summary: extend the Conch parser from a basic command record into typed AST nodes for the primary command families, then peel parser- and validation-oriented logic out of `conch.cc` into dedicated modules.
- Alternatives considered: continuing to improve the token-vector approach incrementally was rejected because it keeps complexity concentrated in the shell loop.

## Acceptance Criteria

- Parser tests cover typed AST output for the primary command families.
- `conch.cc` no longer owns most command-shape parsing for those families.
- Existing Conch regression tests continue to pass.

## Risks / Open Questions

- Risk: preserving current shell behavior while decomposing the monolith may surface subtle compatibility regressions.
- Question: should command-family handlers be split by feature area or by parse/validate/execute stage first?

## Dependencies

- Dependency 1: AR-0026 Conch Parser Maturity Levels.
- Dependency 2: ER-0023 Conch Parser Implementation.
- Dependency 3: ER-0047.2 Core Command Migration.

## Implementation Notes

- Notes for implementer: prefer small local modules over another large parser or handler monolith; preserve current command text compatibility unless a change is explicitly documented.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - interactive Conch commands with quoted values, aliases, namespace navigation, and IO commands continue to behave as expected.
```
