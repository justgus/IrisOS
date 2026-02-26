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
