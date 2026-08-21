---
Legacy-ID: ER-0032
GitHub-Issue: #113
Source-Path: docs/ER/ER-0032-Astra-Math-Types.md
---

# T-0120 — Astra Math Types

## Task Metadata

- Task ID: T-0120
- Legacy ID: ER-0032
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #113
- AR Dependencies: -
- Date Requested: 2026-02-21
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0032-Astra-Math-Types.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #113
---


# ER-0032 — Astra Math Types

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0032
- Title: Astra Math Types
- Status: Verified
- Date: 2026-02-21
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: IrisOS requires Float/Double and higher-order math structures.
- Background / constraints: Must integrate with Crate collections where applicable.

## Goals

- Add Float and Double to Refract core types.
- Define Vector/Matrix/Tensor with shape metadata.

## Non-Goals

- Full linear algebra library or GPU acceleration.
- Automatic differentiation in v1.

## Scope

- In scope: type definitions and basic operations metadata.
- Out of scope: optimized kernels.

## Requirements

- Functional: math types are registered and introspectable.
- Non-functional: clear shape/element type representation.

## Proposed Approach

- Summary: define math primitives and register them in Refract, leveraging collection
  parameterization for Vector/Matrix/Tensor.
- Alternatives considered: separate math subsystem outside Refract (rejected).

## Acceptance Criteria

- Float/Double appear in core schema.
- Vector/Matrix/Tensor definitions include shape metadata.

## Risks / Open Questions

- Risk: shape/size semantics may vary by use case.
- Question: fixed vs dynamic shapes for v1?

## Dependencies

- Dependency 1: ER-0031 Crate Collections.

## Implementation Notes

- Notes for implementer: prefer explicit shape metadata over implicit conventions.

## Verification Plan

- Tests to run: registry tests for math types.
- Manual checks: `show type` includes shape metadata.
```
