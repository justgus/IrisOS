---
Legacy-ID: ER-0012
GitHub-Issue: #93
Source-Path: docs/ER/ER-0012-Phase4-Demo-Core.md
---

# T-0100 — Phase 4 Milestone 1: Demo Object Core

## Task Metadata

- Task ID: T-0100
- Legacy ID: ER-0012
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #93
- AR Dependencies: -
- Date Requested: 2026-02-25
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0012-Phase4-Demo-Core.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #93
---

# ER-0012 — Phase 4 Milestone 1: Demo Object Core

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0012
- Title: Phase 4 Milestone 1 — Demo Object Core
- Status: Verified
- Date: 2026-02-25
- Owners: Mike
- Type: Enhancement

## Context

Phase 4 begins by implementing the core demo object and minimal artifact emission.

## Goals

- Implement one demo object (PropulsionSynth or TrafficOracle or AlgorithmWorkbench).
- Emit basic artifacts (TextLog, Metric, Table).

## Non-Goals

- Full demo polish.

## Scope

- In scope:
  - Demo object data model.
  - Basic artifact emissions.
- Out of scope:
  - Complex detail levels.

## Requirements

- Functional:
  - Start demo object -> initial Conchos spawn.

## Proposed Approach

- Implement demo object with a simple execution loop.
- Publish artifacts to Referee.

## Acceptance Criteria

- Starting the demo object produces visible artifacts.

## Risks / Open Questions

- Choosing the demo scenario.

## Dependencies

- ER-0011 Phase 3 Integration

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Run demo in Conch.
```
