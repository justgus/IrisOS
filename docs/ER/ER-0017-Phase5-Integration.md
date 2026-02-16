# ER-0017 — Phase 5 Integration: Comms + Reactor

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0017
- Title: Phase 5 Integration — Comms + Reactor
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 5 integration validates Comms primitives and CEO reactor working together end-to-end.

## Goals

- Validate ByteStream operations under reactor control.

## Non-Goals

- Full network protocols.

## Scope

- In scope:
  - End-to-end Comms + reactor integration test.

## Requirements

- Functional:
  - ByteStream send/recv works with await.

## Proposed Approach

- Add integration tests for Comms + reactor.

## Acceptance Criteria

- Comms demo passes under reactor.

## Risks / Open Questions

- Reactor load behavior.

## Dependencies

- ER-0015 Phase 5 Milestone 1
- ER-0016 Phase 5 Milestone 2

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Run Comms demo in Conch (optional).
