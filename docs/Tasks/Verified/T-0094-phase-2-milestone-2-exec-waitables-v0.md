---
Legacy-ID: ER-0006
GitHub-Issue: #87
Source-Path: docs/ER/ER-0006-Phase2-Exec-Waitables.md
---

# T-0094 — Phase 2 Milestone 2: Exec Waitables v0

## Task Metadata

- Task ID: T-0094
- Legacy ID: ER-0006
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #87
- AR Dependencies: -
- Date Requested: 2026-02-16
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0006-Phase2-Exec-Waitables.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #87
---

# ER-0006 — Phase 2 Milestone 2: Exec Waitables v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0006
- Title: Phase 2 Milestone 2 — Exec Waitables v0
- Status: Verified
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 2 requires Waitable/Awaitable primitives that integrate with CEO and park tasks rather than
OS threads.

## Goals

- Implement Waitable/Awaitable abstraction.
- Implement minimal Exec primitives (Event, Semaphore, Mutex, Future) using CEO await.

## Non-Goals

- Advanced scheduling or fairness policies.

## Scope

- In scope:
  - Waitable/Awaitable core.
  - Event/Semaphore/Mutex/Future.
- Out of scope:
  - Full channel/message abstractions (later phases).

## Requirements

- Functional:
  - await parks tasks and resumes on signal.
- Non-functional:
  - Deterministic behavior.

## Proposed Approach

- Define Waitable interface.
- Implement Exec primitives that signal/wake via CEO.

## Acceptance Criteria

- Tasks can await and resume on events.
- Mutex/Semaphore work without blocking OS threads.

## Risks / Open Questions

- Wake ordering and starvation.

## Dependencies

- ER-0005 Phase 2 Milestone 1

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Await/wake in sample tasks.
```
