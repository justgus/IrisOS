---
GitHub-Issue: #TBD
---

# ER-0039 — Structured Arrays and Collection Integration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0039
- Title: Structured Arrays and Collection Integration
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: arrays of structured types must be first-class and consistent with Crate collections.
- Background / constraints: must integrate with generic parameterization and structured type metadata.

## Goals

- Define arrays of structs/packets/enums as first-class collection types.
- Ensure serialization and validation for arrays of structured types.

## Non-Goals

- Advanced collection performance optimizations.
- New collection types beyond those in AR-0017.

## Scope

- In scope: collection metadata and type registration for structured arrays.
- Out of scope: new collection APIs or algorithms.

## Requirements

- Functional: arrays of structured types are registered and introspectable.
- Non-functional: deterministic element type identity for array types.

## Proposed Approach

- Summary: extend collection definitions to accept structured type parameters and register them in Refract.
- Alternatives considered: separate bespoke array types per structured kind (rejected).

## Acceptance Criteria

- `show type` displays arrays of structured types with element metadata.
- Arrays of packets serialize with deterministic element layouts.

## Risks / Open Questions

- Risk: collection parameterization depends on AR-0018 generic encoding.
- Question: what minimal array operations are required for v1?

## Dependencies

- Dependency 1: ER-0038 Structured Types: Struct/Packet/Enum.
- Dependency 2: ER-0031 Crate Collections.

## Implementation Notes

- Notes for implementer: ensure collection type IDs are stable across processes.

## Verification Plan

- Tests to run: registry listing tests for structured arrays.
- Manual checks: instantiate an array of structs and inspect schema.
