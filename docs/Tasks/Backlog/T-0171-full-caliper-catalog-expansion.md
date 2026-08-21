---
Legacy-ID: ER-0075
GitHub-Issue: #283
Source-Path: docs/ER/ER-0075-Full-Caliper-Catalog-Expansion.md
---

# T-0171 — Full Caliper Catalog Expansion

## Task Metadata

- Task ID: T-0171
- Legacy ID: ER-0075
- Status: Backlog
- Source Status: Proposed
- Epic: EP-003
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #283
- AR Dependencies: AR-0019
- Date Requested: 2026-05-28
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0075-Full-Caliper-Catalog-Expansion.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #283
AR-Dependencies: AR-0019
ER-Dependencies: ER-0033
---

# ER-0075 — Full Caliper Catalog Expansion

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0075
- Title: Full Caliper Catalog Expansion
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: ER-0033 added a starter Caliper catalog, but AR-0019 calls for a complete canonical SI and imperial catalog.
- Background / constraints: Catalog data must remain deterministic and based on authoritative definitions.

## Goals

- Expand the canonical SI base, derived, and prefixed unit catalog.
- Add common imperial and US customary units.
- Record canonical names, symbols, dimensions, and conversion metadata placeholders.

## Non-Goals

- Runtime conversion execution.
- Conch conversion commands.
- Domain-specific catalogs outside core SI and imperial units.

## Scope

- In scope: catalog entries, dimensions, symbols, and coverage tests.
- Out of scope: conversion engine behavior and user-defined override precedence.

## Requirements

- Functional: canonical catalog includes SI base and common derived units.
- Functional: canonical catalog includes common imperial/US customary units.
- Functional: duplicate symbols or ambiguous unit definitions are rejected.
- Non-functional: catalog ordering and serialization are deterministic.

## Proposed Approach

- Summary: expand the existing Caliper catalog data model and tests before implementing runtime conversion.
- Alternatives considered: implementing conversions first was rejected because conversion behavior depends on catalog completeness.

## Acceptance Criteria

- Tests verify expected SI and imperial unit coverage.
- Tests verify duplicate or ambiguous entries are rejected.
- Existing Caliper starter catalog tests continue to pass.

## Risks / Open Questions

- Risk: catalog coverage can become subjective without an explicit source list.
- Question: which authoritative references should be cited in the implementation notes for each conversion factor?

## Dependencies

- Dependency 1: AR-0019 Caliper Unit Catalog.
- Dependency 2: ER-0033 Caliper Units and Quantities.

## Implementation Notes

- Notes for implementer: keep source references visible for non-obvious conversion constants.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect catalog coverage for SI base, SI derived, metric prefixes, and common imperial units.
```
