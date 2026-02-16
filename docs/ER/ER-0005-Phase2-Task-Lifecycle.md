# ER-0005 — Phase 2 Milestone 1: CEO Task Lifecycle v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0005
- Title: Phase 2 Milestone 1 — CEO Task Lifecycle v0
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 2 starts by implementing the CEO task model and supervision tree.

## Goals

- Implement TaskID, states, parent/child relationships.
- Implement spawn, cancel, kill, and task registry.

## Non-Goals

- Advanced scheduling policies.

## Scope

- In scope:
  - Task lifecycle + supervision tree.
  - Task state transitions.
  - Task registry for Conch/Vizier.
- Out of scope:
  - QoS and resource quotas.

## Requirements

- Functional:
  - spawn, cancel, kill.
  - parent/child tracking.
- Non-functional:
  - Deterministic behavior.

## Proposed Approach

- Implement Task object model with states.
- Provide task registry API.

## Acceptance Criteria

- Tasks can be spawned and terminated.
- Supervision tree relationships are tracked.

## Risks / Open Questions

- Cancellation propagation semantics.

## Dependencies

- AR-0006 CEO/Exec Runtime Model

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Spawn tasks and observe state transitions.
