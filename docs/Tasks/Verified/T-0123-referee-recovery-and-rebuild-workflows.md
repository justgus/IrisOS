---
Legacy-ID: ER-0035
GitHub-Issue: #171
Source-Path: docs/ER/ER-0035-Referee-Recovery-and-Rebuild-Workflows.md
---

# T-0123 — Referee Recovery and Rebuild Workflows

## Task Metadata

- Task ID: T-0123
- Legacy ID: ER-0035
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #171
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0035-Referee-Recovery-and-Rebuild-Workflows.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #171
---

# ER-0035 — Referee Recovery and Rebuild Workflows

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0035
- Title: Referee Recovery and Rebuild Workflows
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v1 persistence requires validated recovery and rebuild workflows after crashes.
- Background / constraints: Must align with the segment/index storage layout in AR-0003/AR-0004.

## Goals

- Implement rebuild procedures for indexes from segment files.
- Provide deterministic recovery steps on startup.

## Non-Goals

- Distributed replication or multi-node recovery.
- Full administrative UI for recovery operations.

## Scope

- In scope: rebuild logic, validation checks, and startup recovery flow.
- Out of scope: full backup/restore tooling.

## Requirements

- Functional: detect inconsistent indexes and rebuild them.
- Non-functional: deterministic, repeatable recovery steps.

## Proposed Approach

- Summary: add a recovery pass during startup that validates segments and rebuilds indexes when needed.
- Alternatives considered: manual rebuild-only workflows (rejected for kernel reliability).

## Acceptance Criteria

- Recovery handles interrupted writes and missing indexes.
- Rebuild completes without data loss for valid segments.

## Risks / Open Questions

- Risk: recovery time may grow without bounded index rebuild strategies.
- Question: what minimum metadata is required to validate a segment?

## Dependencies

- Dependency 1: ER-0034 Referee Storage Layout Implementation (v1).

## Implementation Notes

- Notes for implementer: record recovery actions in a minimal log for diagnostics.

## Verification Plan

- Tests to run: simulated crash tests with index corruption.
- Manual checks: trigger rebuild and verify object/edge queries.
```
