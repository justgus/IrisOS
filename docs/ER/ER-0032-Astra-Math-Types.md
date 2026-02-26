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
