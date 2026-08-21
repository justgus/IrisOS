---
Legacy-ID: ER-0033
GitHub-Issue: #114
Source-Path: docs/ER/ER-0033-Caliper-Units-and-Quantities.md
---

# T-0121 — Caliper Units and Quantities

## Task Metadata

- Task ID: T-0121
- Legacy ID: ER-0033
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #114
- AR Dependencies: -
- Date Requested: 2026-02-21
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0033-Caliper-Units-and-Quantities.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #114
---


# ER-0033 — Caliper Units and Quantities

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0033
- Title: Caliper Units and Quantities
- Status: Verified
- Date: 2026-02-21
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: Units and quantities are needed to avoid ambiguity in computation.
- Background / constraints: AR-0017 defines units and quantities as first-class types.

## Goals

- Define Unit and Dimension schemas in Refract.
- Add quantity types (Angle, Duration, Span, Range, Percentage, Ratio).
- Provide conversion metadata and compatibility checks.

## Non-Goals

- Full physical constants library.
- Automatic symbolic unit simplification beyond basic conversions.

## Scope

- In scope: schema definitions, conversion metadata, basic validation rules.
- Out of scope: runtime conversion engines beyond metadata.

## Requirements

- Functional: units and quantities are registered and introspectable.
- Non-functional: conversion rules are deterministic and explicit.

## Proposed Approach

- Summary: define Unit/Dimension objects and attach unit metadata to quantity types.
- Alternatives considered: unit handling outside Refract (rejected).

## Acceptance Criteria

- Unit and Dimension appear in Refract types.
- Quantity types are listed with unit metadata.

## Risks / Open Questions

- Risk: unit taxonomy can grow quickly.
- Question: what minimal unit set is required for v1?

## Dependencies

- Dependency 1: ER-0031 Crate Collections.
- Dependency 2: ER-0032 Astra Math Types.

## Implementation Notes

- Notes for implementer: keep unit sets small and extensible.

## Verification Plan

- Tests to run: registry tests for unit and quantity types.
- Manual checks: `show type` for units and quantities.
```
