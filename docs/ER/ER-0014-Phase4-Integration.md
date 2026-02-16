# ER-0014 — Phase 4 Integration: Demo End-to-End

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0014
- Title: Phase 4 Integration — Demo End-to-End
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 4 integration ensures the demo object and composite summary pattern work end-to-end in
Conch.

## Goals

- Validate demo flow from start to expanded details.

## Non-Goals

- Demo polish or advanced theming.

## Scope

- In scope:
  - End-to-end demo run.
- Out of scope:
  - Additional demo scenarios.

## Requirements

- Functional:
  - Demo shows organic Concho growth.

## Proposed Approach

- Add a demo script and basic smoke tests.

## Acceptance Criteria

- Demo run matches the IrisOS v0 memo.

## Risks / Open Questions

- Demo flakiness.

## Dependencies

- ER-0012 Phase 4 Milestone 1
- ER-0013 Phase 4 Milestone 2

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Run demo in Conch and observe output.
