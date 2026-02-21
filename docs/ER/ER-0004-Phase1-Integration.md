---
GitHub-Issue: N/A
---

# ER-0004 — Phase 1 Integration: Schema Registry + Referee Graph

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0004
- Title: Phase 1 Integration — Schema Registry + Referee Graph
- Status: Verified
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 1 integration ensures schemas and Referee graph enhancements work together and are
bootstrapped deterministically.

## Goals

- Provide deterministic bootstrap path for schemas.
- Mirror C++ schema definitions into Referee at boot.
- Ensure schemas are queryable after bootstrap.

## Non-Goals

- Full schema authoring UI.
- Schema migration tooling beyond bootstrap.

## Scope

- In scope:
  - C++ schema registry definitions.
  - Boot-time export into Referee.
  - Idempotent bootstrap.
- Out of scope:
  - Editing schemas in Conch.

## Requirements

- Functional:
  - Bootstrap creates schemas if missing.
  - Re-running bootstrap does not duplicate schemas.
- Non-functional:
  - Deterministic order and stable IDs.

## Proposed Approach

- Define static schema registry in code.
- On boot, check for schema presence by TypeID/DefinitionID.
- Insert missing schemas and link relationships.

## Acceptance Criteria

- Bootstrapped schemas appear in Referee and can be queried.
- Re-running bootstrap does not create duplicates.

## Risks / Open Questions

- Maintaining stable TypeID/DefinitionID mapping in code.

## Dependencies

- ER-0002 Phase 1 Milestone 1
- ER-0003 Phase 1 Milestone 2

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Run bootstrap twice and verify no duplicates.
