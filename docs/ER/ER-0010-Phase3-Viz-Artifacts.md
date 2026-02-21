---
GitHub-Issue: N/A
---

# ER-0010 — Phase 3 Milestone 3: Viz Artifacts v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0010
- Title: Phase 3 Milestone 3 — Viz Artifacts v0
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 3 requires concrete Viz artifacts so producers can publish UI-worthy objects without doing
rendering.

## Goals

- Implement Viz::Panel, TextLog, Metric, Table, Tree (v0).
- Define artifact data models and relationships.

## Non-Goals

- Full image rendering or GUI features.

## Scope

- In scope:
  - Data-only artifact objects.
  - produced/progress/diagnostic relationships.
- Out of scope:
  - GUI styling.

## Requirements

- Functional:
  - Artifacts can be stored in Referee.
- Non-functional:
  - Deterministic serialization.

## Proposed Approach

- Define Viz artifacts as Refract schemas and C++ types.
- Store artifacts in Referee with edges to producers.

## Acceptance Criteria

- Artifacts exist as objects and can be queried.

## Risks / Open Questions

- Artifact schema evolution.

## Dependencies

- ER-0002 Phase 1 Milestone 1
- ER-0003 Phase 1 Milestone 2

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Create a Viz::TextLog and verify retrieval.
