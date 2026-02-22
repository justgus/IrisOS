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
- Status: Draft
- Date: 2026-02-16
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
