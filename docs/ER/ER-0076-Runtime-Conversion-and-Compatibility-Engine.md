---
GitHub-Issue: N/A
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075
---

# ER-0076 — Runtime Conversion and Compatibility Engine

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0076
- Title: Runtime Conversion and Compatibility Engine
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Caliper unit metadata is useful for introspection, but AR-0019 requires deterministic runtime conversion and compatibility evaluation.
- Background / constraints: ER-0075 should provide the expanded catalog needed by the conversion engine.

## Goals

- Add runtime unit compatibility checks based on dimensions.
- Add deterministic conversion chains between compatible units.
- Support scale and offset conversions where the catalog defines them.

## Non-Goals

- Automatic symbolic simplification beyond explicit metadata.
- Domain-specific conversion catalogs.
- Conch command surfaces.

## Scope

- In scope: conversion API, compatibility checks, conversion-chain tests, and failure behavior.
- Out of scope: UI/Conch commands and user-defined catalog overrides unless needed by accepted catalog structure.

## Requirements

- Functional: compatible units can be converted deterministically.
- Functional: incompatible dimensions are rejected.
- Functional: offset conversions such as temperature are handled explicitly.
- Non-functional: conversion results are stable across reloads.

## Proposed Approach

- Summary: implement a conversion engine over the canonical catalog with dimension-based compatibility and explicit chain resolution.
- Alternatives considered: relying on ad hoc per-unit conversion code was rejected because it would bypass Caliper metadata.

## Acceptance Criteria

- Tests verify direct and chained conversions.
- Tests verify incompatible units are rejected.
- Tests verify offset conversions behave deterministically.

## Risks / Open Questions

- Risk: floating-point precision policy needs clear test tolerances.
- Question: should rational conversion factors be stored exactly before conversion to runtime numeric types?

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.

## Implementation Notes

- Notes for implementer: make precision and rounding policy explicit in tests.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - convert representative length, mass, time, and temperature values.
