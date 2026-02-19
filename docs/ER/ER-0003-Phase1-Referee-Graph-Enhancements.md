# ER-0003 — Phase 1 Milestone 2: Referee Graph Enhancements v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0003
- Title: Phase 1 Milestone 2 — Referee Graph Enhancements v0
- Status: Verified
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 1 requires Referee to store objects with DefinitionID, and to store/query relationship
edges by name/role with indexes for Conch and Refract lookups.

## Goals

- Persist objects with ObjectID + DefinitionID (and TypeID if needed).
- Persist edges with name/role metadata.
- Add indexes for TypeID/DefinitionID and edge name/role.

## Non-Goals

- Full segment/index storage (SQLite is acceptable for now).
- Advanced query language.

## Scope

- In scope:
  - Schema changes to store DefinitionID with object records.
  - Edge schema extended with name/role.
  - Indexes for object type/definition and edge lookups.
- Out of scope:
  - Cross-node replication.

## Requirements

- Functional:
  - Create/read objects with DefinitionID.
  - Query edges by fromID, toID, and (fromID, name/role).
- Non-functional:
  - Deterministic storage.
  - Clear migration from existing schema (SQLite).

## Proposed Approach

- Extend SQLite schema to include DefinitionID (BLOB) for objects.
- Extend edges table with name/role columns (if not already present).
- Add indexes for DefinitionID and edge lookups.

## Acceptance Criteria

- Object creation stores DefinitionID.
- Edge queries by name/role return correct results.
- Tests pass for edge indexing paths.

## Risks / Open Questions

- Schema migration needed for existing data.

## Dependencies

- AR-0009 Referee Object Store
- ER-0002 Phase 1 Milestone 1

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Create object + edges and verify queries.

## Implementation Notes

- Sprint 4 review: functionality appears implemented in `src/referee_sqlite/sqlite_store.cc` and
  covered by `tests/test_referee_core.cc`. Status remains Draft until System Engineer verification.
