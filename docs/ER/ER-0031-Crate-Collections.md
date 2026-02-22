---
GitHub-Issue: #112
---


# ER-0031 — Crate Collections

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0031
- Title: Crate Collections
- Status: Proposed
- Date: 2026-02-21
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: IrisOS needs first-class collection types for data organization.
- Background / constraints: AR-0017 defines core collection types and parameterization.

## Goals

- Define collection types in Refract (Array, List, Set, Map, Tuple, Bytes).
- Provide baseline operations (size, iterate, index, contains).

## Non-Goals

- Full STL-equivalent API surface.
- Advanced performance optimizations in v1.

## Scope

- In scope: Refract schemas, type registration, minimal ops.
- Out of scope: serialization formats beyond existing mechanisms.

## Requirements

- Functional: collection types are registered and introspectable.
- Non-functional: parameterization is explicit and consistent.

## Proposed Approach

- Summary: introduce parameterized collection definitions in Refract and register them
  during bootstrap.
- Alternatives considered: separate unparameterized collection types (defer).

## Acceptance Criteria

- Collections appear in Refract type listings.
- Operations are defined for basic usage.

## Risks / Open Questions

- Risk: parameterized types add complexity to Refract type IDs.
- Question: how are type parameters encoded and stored?

## Dependencies

- Dependency 1: AR-0017 Collections, Math, and Units.

## Implementation Notes

- Notes for implementer: ensure core bootstrap remains deterministic.

## Verification Plan

- Tests to run: registry listing tests for collection types.
- Manual checks: `show type` for collections.
