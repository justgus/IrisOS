---
GitHub-Issue: N/A
---

# ER-0059 — CEO Task Capability Attachment

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0059
- Title: CEO Task Capability Attachment
- Status: Complete
- Date: 2026-05-26
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: ER-0058 added persisted capability context objects, but CEO task records cannot yet reference those contexts as task metadata.
- Background / constraints: AR-0022 Stage D calls for capability-aware service context. This ER attaches capability context identity to tasks without introducing service-boundary enforcement.

## Goals

- Allow CEO task records to carry an optional capability context object ID.
- Preserve capability context attachment through normal task lifecycle transitions.
- Allow task listing by attached capability context.

## Non-Goals

- Capability enforcement.
- Policy evaluation.
- Sandbox isolation.
- Validation that an attached context exists in Referee.

## Scope

- In scope: task metadata field for capability context object ID.
- In scope: attach, clear, lookup, profile snapshot, and list-by-context behavior.
- In scope: deterministic tests for attachment, lifecycle stability, and terminal-task rejection.
- Out of scope: service-boundary authorization checks and capability grant semantics.

## Requirements

- Functional: non-terminal tasks can attach and clear a capability context ID.
- Functional: task lookup, task listing, and task profile snapshots expose the attached context ID.
- Functional: task lifecycle transitions do not drop the attached context ID.
- Functional: terminal tasks reject attachment changes.
- Non-functional: existing task behavior remains stable for tasks without an attached capability context.

## Proposed Approach

- Summary: add an optional capability context object ID to CEO task records and expose small registry helpers for attach, clear, and list-by-context operations.
- Alternatives considered: embedding full capability grant data in task records was rejected because ER-0058 made capability context a reusable persisted object.

## Acceptance Criteria

- Tests verify capability context attachment is visible through task lookup.
- Tests verify list-by-context returns attached tasks.
- Tests verify profile snapshots include the attached context ID.
- Tests verify lifecycle transitions preserve the attachment.
- Tests verify terminal tasks reject attachment changes.

## Risks / Open Questions

- Risk: callers may mistake attachment for enforcement.
- Question: should ER-0060 validate attached context existence before service boundary checks?

## Dependencies

- Dependency 1: ER-0058 Capability Context Objects and Persistence.
- Dependency 2: ER-0057 Service Host Lifecycle and Persistent Registry.
- Dependency 3: AR-0022 Staged Service Plane Delivery.

## Implementation Notes

- Notes for implementer: keep this ER metadata-only. Enforcement belongs to ER-0060.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create a task, attach a capability context ID, inspect the task record and profile snapshot, then run lifecycle transitions.
