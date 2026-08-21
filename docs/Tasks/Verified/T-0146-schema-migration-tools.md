---
Legacy-ID: ER-0050
GitHub-Issue: #186
Source-Path: docs/ER/ER-0050-Schema-Migration-Tools.md
---

# T-0146 — Schema Migration Tools

## Task Metadata

- Task ID: T-0146
- Legacy ID: ER-0050
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #186
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0050-Schema-Migration-Tools.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #186
---

# ER-0050 — Schema Migration Tools

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0050
- Title: Schema Migration Tools
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v2 needs tooling to apply Definition version migrations safely.
- Background / constraints: leverage migration hooks from v1.

## Goals

- Provide CLI tools to list and apply migrations for Definitions.
- Validate migrations against stored object graphs.

## Non-Goals

- Fully automated semantic transformations for arbitrary schemas.
- Distributed migrations across multiple stores.

## Scope

- In scope: migration tooling, validation checks, reporting.
- Out of scope: complex transform language or GUI tools.

## Requirements

- Functional: list available migrations and apply them to objects.
- Non-functional: deterministic, auditable migration steps.

## Proposed Approach

- Summary: implement a migration runner that uses Definition hooks and records results.
- Alternatives considered: manual migration scripts only (rejected).

## Acceptance Criteria

- Migrations can be listed, applied, and verified.
- Migration results are recorded with before/after metadata.

## Risks / Open Questions

- Risk: migrations may fail without a rollback plan.
- Question: what minimal rollback mechanism is required for v2?

## Dependencies

- Dependency 1: ER-0036 Definition Versioning and Migration Hooks.

## Implementation Notes

- Notes for implementer: record migration history in Referee.
- Implementation details:
  - Added `migrate list`, `migrate apply`, and `migrate verify` Conch commands.
  - Migration records are stored as `Refract::MigrationRecord` objects with before/after metadata.
  - Migrations apply supersedes chains and reuse the existing payloads, recording hook names where provided.

## Verification Plan

- Tests to run: migration tool tests with sample schemas.
- Manual checks: apply a migration to a demo graph and verify.
```
