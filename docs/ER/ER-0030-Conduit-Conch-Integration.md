---
GitHub-Issue: #111
---


# ER-0030 — Conduit Conch Integration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0030
- Title: Conduit Conch Integration
- Status: Implemented
- Date: 2026-02-25
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: Conch needs to list and validate operations with scope, overloads, and
  inheritance.
- Background / constraints: Must use the Conduit registry/dispatch APIs.

## Goals

- Add Conch commands to list operations by scope and inheritance.
- Validate `call` using Conduit dispatch.

## Non-Goals

- Execution runtime changes.

## Scope

- In scope: `show type` and `call` integration, listing options.
- Out of scope: UI and visualization updates.

## Requirements

- Functional: Conch can list declared/inherited operations and overload sets.
- Non-functional: clear error messages for ambiguous calls.

## Proposed Approach

- Summary: extend Conch to query Conduit for operation lists and dispatch results.
- Alternatives considered: leave Conch unchanged (rejected).

## Acceptance Criteria

- `show type` includes operations grouped by scope and overloads.
- `call` validates against resolved operation signature.

## Risks / Open Questions

- Risk: changes to Conch output formats may impact tooling.
- Question: should Conch expose a dedicated `ops` command?

## Dependencies

- Dependency 1: ER-0028 Conduit Operation Model.
- Dependency 2: ER-0029 Conduit Dispatch Engine.

## Implementation Notes

- Notes for implementer: keep output stable and additive.

## Verification Plan

- Tests to run: Conch integration tests if available.
- Manual checks: list operations and validate a call.
