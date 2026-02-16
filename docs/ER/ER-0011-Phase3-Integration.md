# ER-0011 — Phase 3 Integration: Conch + Vizier + Viz

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0011
- Title: Phase 3 Integration — Conch + Vizier + Viz
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 3 integration ensures Conch, Vizier, and Viz artifacts work end-to-end.

## Goals

- Conch auto-spawns Conchos from artifacts.
- Routing uses Refract metadata when present.

## Non-Goals

- Complex layout behavior.

## Scope

- In scope:
  - End-to-end artifact pipeline.
- Out of scope:
  - Advanced UI features.

## Requirements

- Functional:
  - Start object -> artifacts -> Conchos.
- Non-functional:
  - Deterministic behavior.

## Proposed Approach

- Add integration tests for artifact flow.

## Acceptance Criteria

- Conch shows Conchos for produced artifacts.

## Risks / Open Questions

- UI ergonomics.

## Dependencies

- ER-0008 Phase 3 Milestone 1
- ER-0009 Phase 3 Milestone 2
- ER-0010 Phase 3 Milestone 3

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Start demo object and observe views.
