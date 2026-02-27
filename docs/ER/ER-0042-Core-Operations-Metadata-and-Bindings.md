---
GitHub-Issue: #TBD
---

# ER-0042 — Core Operations Metadata and Bindings

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0042
- Title: Core Operations Metadata and Bindings
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0020 defines core operations but they are not yet bound to implementations.
- Background / constraints: must integrate with Conduit operation model and dispatch.

## Goals

- Register core operations (`to_string`, `print`, `render`, compare) for primitives.
- Bind operation definitions to concrete implementations.

## Non-Goals

- Full operator overloading across all user types.
- GUI rendering beyond `render` hook semantics.

## Scope

- In scope: core primitive operation metadata and bindings.
- Out of scope: rich rendering systems or UI frameworks.

## Requirements

- Functional: core primitives expose operations via Refract and Conduit.
- Non-functional: deterministic output for `to_string` and `print`.

## Proposed Approach

- Summary: add operation metadata for core primitives and bind to implementation functions.
- Alternatives considered: implicit operations without metadata (rejected).

## Acceptance Criteria

- `show type` lists core operations for primitives.
- Conch `call` can invoke core operations via Conduit.

## Risks / Open Questions

- Risk: missing compare semantics could break ordering assumptions.
- Question: what is the exact signed type used for compare?

## Dependencies

- Dependency 1: ER-0028 Conduit Operation Model.
- Dependency 2: ER-0029 Conduit Dispatch Engine.
- Dependency 3: ER-0030 Conduit Conch Integration.

## Implementation Notes

- Notes for implementer: keep bindings centralized and auditable.

## Verification Plan

- Tests to run: operation invocation tests for core primitives.
- Manual checks: Conch `call` of `to_string` and `compare`.
