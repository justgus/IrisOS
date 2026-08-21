---
Legacy-ID: ER-0077
GitHub-Issue: #285
Source-Path: docs/ER/ER-0077-Conch-Conversion-and-Inspection-Commands.md
---

# T-0173 — Conch Conversion and Inspection Commands

## Task Metadata

- Task ID: T-0173
- Legacy ID: ER-0077
- Status: Backlog
- Source Status: Proposed
- Epic: EP-003
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #285
- AR Dependencies: AR-0019
- Date Requested: 2026-05-28
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0077-Conch-Conversion-and-Inspection-Commands.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #285
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075, ER-0076
---

# ER-0077 — Conch Conversion and Inspection Commands

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0077
- Title: Conch Conversion and Inspection Commands
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Caliper catalog and conversion behavior need a user-facing Conch surface for listing, lookup, and conversion.
- Background / constraints: ER-0075 and ER-0076 should provide the catalog and runtime conversion engine.

## Goals

- Add Conch commands for unit listing and lookup.
- Add Conch commands for converting values between compatible units.
- Surface deterministic errors for unknown units and incompatible dimensions.

## Non-Goals

- Full scripting integration.
- Domain-specific catalog authoring UI.
- Rich table rendering beyond existing Conch output conventions.

## Scope

- In scope: command parsing, execution, output formatting, and tests.
- Out of scope: new catalog data and conversion engine internals.

## Requirements

- Functional: users can list units and inspect unit metadata.
- Functional: users can convert values between compatible units.
- Functional: unknown units and incompatible conversions produce clear errors.
- Non-functional: commands follow existing Conch naming and error conventions.

## Proposed Approach

- Summary: add thin Conch command wrappers over the Caliper catalog and conversion APIs.
- Alternatives considered: exposing conversion only through tests or libraries was rejected because AR-0019 calls for Conch and runtime API access.

## Acceptance Criteria

- Tests verify unit listing and lookup commands.
- Tests verify successful conversion command output.
- Tests verify unknown and incompatible unit errors.

## Risks / Open Questions

- Risk: command names may conflict with future parser grammar expansion.
- Question: should conversion command output include dimension metadata by default or only on inspection commands?

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.
- Dependency 2: ER-0076 Runtime Conversion and Compatibility Engine.

## Implementation Notes

- Notes for implementer: keep command output consistent with existing Conch inspection commands.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - list units, inspect a unit, convert compatible units, and attempt an incompatible conversion.
```
