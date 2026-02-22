---
GitHub-Issue: #99
---

# ER-0018 — Phase 6 Milestone 1: Storage Alignment

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0018
- Title: Phase 6 Milestone 1 — Storage Alignment
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 6 begins by aligning Referee storage with AR-0003/AR-0004 decisions (SQLite vs
segment/index) and capturing the decision in docs.

## Goals

- Decide on storage strategy for v0.
- Update ARs if needed.

## Non-Goals

- Full segment/index implementation if not chosen.

## Scope

- In scope:
  - Documented storage decision.
- Out of scope:
  - Major storage refactor unless explicitly approved.

## Requirements

- Functional:
  - Storage decision recorded.

## Proposed Approach

- Review current SQLite implementation vs ARs.
- Update ARs to reflect the chosen strategy.

## Acceptance Criteria

- Storage decision documented and consistent.

## Risks / Open Questions

- Long-term divergence if SQLite remains.

## Dependencies

- ER-0004 Phase 1 Integration

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - None.
