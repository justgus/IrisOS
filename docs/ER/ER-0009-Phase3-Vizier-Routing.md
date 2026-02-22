---
GitHub-Issue: N/A
---

# ER-0009 — Phase 3 Milestone 2: Vizier Routing v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0009
- Title: Phase 3 Milestone 2 — Vizier Routing v0
- Status: Verified
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 3 needs a routing layer that maps artifacts to Concho views based on type and Refract
metadata.

## Goals

- Implement Vizier routing rules.
- Use Refract metadata for preferred renderers when available.

## Non-Goals

- Pluggable renderer ecosystem.

## Scope

- In scope:
  - Type-based routing.
  - Produced/progress/diagnostic edge triggers.
- Out of scope:
  - Advanced policy-driven routing.

## Requirements

- Functional:
  - Artifacts cause appropriate Conchos to spawn.
- Non-functional:
  - Deterministic routing.

## Proposed Approach

- Build a minimal routing table.
- Use Refract preferred renderer metadata when present.

## Acceptance Criteria

- TextLog/Metric/Table/Tree artifacts map to Conchos.

## Risks / Open Questions

- Handling unknown artifact types.

## Dependencies

- ER-0008 Phase 3 Milestone 1

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Emit artifacts and observe Conchos.
