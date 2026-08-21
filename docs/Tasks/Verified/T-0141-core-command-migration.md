---
Legacy-ID: ER-0047.2
GitHub-Issue: #182
Source-Path: docs/ER/ER-0047.2-Core-Command-Migration.md
---

# T-0141 — Core Command Migration

## Task Metadata

- Task ID: T-0141
- Legacy ID: ER-0047.2
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #182
- AR Dependencies: -
- Date Requested: 2026-03-07
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0047.2-Core-Command-Migration.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #182
---

# ER-0047.2 — Core Command Migration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0047.2
- Title: Core Command Migration
- Status: Verified
- Date: 2026-03-07
- Owners: Mike
- Type: Enhancement

## Context

- We need a manageable first pass to migrate core Conch commands to session operations.

## Goals

- Migrate core commands to session operations and dispatcher execution.
- Keep behavior identical to existing built-ins.

## Non-Goals

- Full migration of every Conch command.
- Refactoring command implementations beyond what is necessary for dispatch.

## Scope

- In scope: `ls`, `show`, `ops`, `call`, `new`, `define`, `find`, `objects`, `edges`.
- In scope: `caps`, `start`, `ps`, `kill`, `task`, `io`.
- Out of scope: demo and viz emission commands (handled in later sub-ERs).

## Requirements

- Functional: core commands work through session operation dispatch.
- Functional: aliases map CLI names to session ops.
- Non-functional: output remains stable.

## Proposed Approach

- Add session operations for the core command set.
- Map CLI tokens to session operations via aliases.
- Route execution through the session operation handlers.

## Acceptance Criteria

- Core commands execute via session operations with no output regressions.
- Conch tests cover the migrated commands.

## Dependencies

- ER-0047.1 Conch Session Operation Model.

## Verification Plan

- Tests: Conch command tests and regression checks for migrated commands.
```
