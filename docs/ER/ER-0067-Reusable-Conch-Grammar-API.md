---
GitHub-Issue: N/A
AR-Dependencies: AR-0015, AR-0026
ER-Dependencies: ER-0056
---

# ER-0067 — Reusable Conch Grammar API

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0067
- Title: Reusable Conch Grammar API
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: The Conch parser is usable by the shell, but AR-0026 Level 4 requires a reusable command grammar surface for non-interactive consumers.
- Background / constraints: ER-0056 decomposed typed AST and shell parsing. This ER defines the stable API boundary before batch execution and regression harness work.

## Goals

- Expose reusable parser entry points outside the interactive shell loop.
- Preserve typed AST and parse-error behavior from ER-0056.
- Document ownership and lifetime expectations for parser inputs and outputs.

## Non-Goals

- Batch execution.
- New scripting semantics.
- Backward-incompatible grammar changes.

## Scope

- In scope: parser API, headers, tests, and concise developer-facing grammar boundary notes.
- Out of scope: shell command execution changes and userland tool integration.

## Requirements

- Functional: non-shell callers can parse command input into the same typed AST used by Conch.
- Functional: parse errors remain location-aware and deterministic.
- Non-functional: existing interactive shell parsing behavior remains stable.

## Proposed Approach

- Summary: factor or expose the existing parser entry points through a small API that can be used by later batch and tooling ERs.
- Alternatives considered: duplicating parser behavior in userland tools was rejected because it would split the grammar source of truth.

## Acceptance Criteria

- Tests verify a non-shell caller can parse representative command families.
- Tests verify parse errors match shell parser behavior.
- Existing Conch parser tests continue to pass.

## Risks / Open Questions

- Risk: exposing too much AST detail could freeze internal parser structures early.
- Question: should the first API return concrete AST nodes or a narrow parse result facade?

## Dependencies

- Dependency 1: AR-0026 Conch Parser Maturity Levels.
- Dependency 2: ER-0056 Conch Typed AST and Shell Decomposition.

## Implementation Notes

- Notes for implementer: prefer a small facade if existing AST ownership is still changing.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - parse representative commands through the reusable API outside the shell loop.
