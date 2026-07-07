---
Legacy-ID: ER-0008
GitHub-Issue: #89
Source-Path: docs/ER/ER-0008-Phase3-Conch-Shell.md
---

# T-0096 — Phase 3 Milestone 1: Conch Shell v0

## Task Metadata

- Task ID: T-0096
- Legacy ID: ER-0008
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #89
- AR Dependencies: -
- Date Requested: 2026-02-16
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0008-Phase3-Conch-Shell.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #89
---

# ER-0008 — Phase 3 Milestone 1: Conch Shell v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0008
- Title: Phase 3 Milestone 1 — Conch Shell v0
- Status: Verified
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 3 begins with a terminal-first Conch shell that can discover and invoke object operations.

## Goals

- Implement core Conch commands (ls, show, edges, find type, call, start, ps, kill).
- Integrate Refract-based introspection.

## Non-Goals

- Complex UI layout or theming.

## Scope

- In scope:
  - Command parsing and dispatch.
  - Object lookup and inspection.
- Out of scope:
  - Advanced piping.

## Requirements

- Functional:
  - Commands resolve objects via Refract.
- Non-functional:
  - Deterministic output.

## Proposed Approach

- Implement a minimal command loop and dispatch table.
- Use Referee + Refract registry for resolution.

## Acceptance Criteria

- Core commands work for basic objects.

## Risks / Open Questions

- Command surface could grow rapidly.

## Dependencies

- ER-0002 Phase 1 Milestone 1
- ER-0003 Phase 1 Milestone 2
- ER-0005 Phase 2 Milestone 1

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Use Conch to list and inspect objects.
```
