---
GitHub-Issue: #109
---


# ER-0028 — Conduit Operation Model

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0028
- Title: Conduit Operation Model
- Status: Proposed
- Date: 2026-02-21
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: IrisOS needs a formal, introspectable operation model to support
  class/object operations, overloads, and polymorphism as defined by AR-0016.
- Background / constraints: Must integrate with Refract definitions and Referee storage.

## Goals

- Define Refract schemas for OperationDefinition and Signature with ordered inputs/outputs.
- Provide a consistent operation registry keyed by type and scope.

## Non-Goals

- Runtime execution engine implementation (separate ER if needed).
- Full dynamic language semantics.

## Scope

- In scope: Refract schema objects, storage format, registry APIs.
- Out of scope: Conch parsing and dispatch algorithm details.

## Requirements

- Functional: operations can be listed by type and scope, including inherited entries.
- Non-functional: stable IDs and deterministic listing order.

## Proposed Approach

- Summary: extend Refract schema to store OperationDefinitions with ordered input/output
  lists and optionality markers.
- Alternatives considered: keep operations as ad-hoc metadata (rejected).

## Acceptance Criteria

- Operations are stored in Refract with full signature metadata.
- Registry can list declared and inherited operations by scope.

## Risks / Open Questions

- Risk: schema changes may require migrations later.
- Question: how are parameter names and ordering represented in the schema?

## Dependencies

- Dependency 1: AR-0016 Operations and Dispatch Model.

## Implementation Notes

- Notes for implementer: keep schema minimal and versioned.

## Verification Plan

- Tests to run: schema unit tests, registry list tests.
- Manual checks: `show type` lists operations with ordered inputs/outputs.
