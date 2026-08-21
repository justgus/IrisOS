---
Legacy-ID: ER-0047.3
GitHub-Issue: #182
Source-Path: docs/ER/ER-0047.3-Utility-Command-Migration.md
---

# T-0142 — Utility Command Migration

## Task Metadata

- Task ID: T-0142
- Legacy ID: ER-0047.3
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #182
- AR Dependencies: -
- Date Requested: 2026-03-07
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0047.3-Utility-Command-Migration.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #182
---

# ER-0047.3 — Utility Command Migration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0047.3
- Title: Utility Command Migration
- Status: Verified
- Date: 2026-03-07
- Owners: Mike
- Type: Enhancement

## Context

- Remaining commands (demo, viz, routing) need migration after core commands are stable.

## Goals

- Migrate remaining Conch commands to session operations.
- Preserve behavior and output.

## Non-Goals

- Introducing new utility features beyond current behavior.

## Scope

- In scope: `emit viz`, `demo v1`, `route`, and any remaining shell-only commands.
- Out of scope: new utilities not already in Conch.

## Requirements

- Functional: migrated commands execute through session operations.
- Non-functional: output and error messages remain stable.

## Proposed Approach

- Add session operations for remaining commands.
- Map CLI tokens to session operations via aliases.
- Route execution through session operation handlers.

## Acceptance Criteria

- Remaining commands execute via session operations with no output regressions.
- Tests cover the migrated commands.

## Dependencies

- ER-0047.2 Core Command Migration.

## Verification Plan

- Tests: Conch command tests and regression checks for migrated commands.
```
