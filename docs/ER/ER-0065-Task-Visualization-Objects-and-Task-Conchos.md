---
GitHub-Issue: N/A
AR-Dependencies: AR-0011, AR-0012, AR-0013, AR-0025
ER-Dependencies: ER-0005, ER-0009, ER-0010, ER-0063, ER-0064
---

# ER-0065 — Task Visualization Objects and Task Conchos

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0065
- Title: Task Visualization Objects and Task Conchos
- Status: Complete
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0025 Mode 2 includes task visualization, but the current routing work only covers artifact-oriented relationships.
- Background / constraints: ER-0063 provides graph change replay and ER-0064 provides relationship-pattern routing. Task-specific view models and Task Conchos need a separate scope before ER-0066 wires observer-driven session growth to task routes.

## Goals

- Define task visualization objects for task state, progress, diagnostics, and produced artifacts.
- Add Vizier route decisions for task-state relationships without creating Conchos directly.
- Define the first Task Concho model needed by Conch session growth.
- Keep task visualization deterministic and testable without a compositor or live event loop.

## Non-Goals

- Observer-driven Conch session mutation.
- Full layout, focus, compositor, or renderer behavior.
- Task scheduler changes.
- Multi-user or remote task observation.

## Scope

- In scope: task visualization object schema, route decisions for task state, and Task Concho metadata.
- In scope: tests for known task states, unknown task states, and route decision stability.
- Out of scope: session growth from graph changes; that belongs to ER-0066.

## Requirements

- Functional: Vizier can produce a route decision for a known task-state relationship.
- Functional: Task visualization objects can reference task identity, state, related artifacts, and diagnostic objects.
- Functional: unknown task states or incomplete task metadata produce no route rather than an error.
- Functional: Task Concho metadata is sufficient for ER-0066 to create or link a task view later.
- Non-functional: artifact relationship routing from ER-0064 remains unchanged.

## Proposed Approach

- Summary: add a small task visualization model above existing CEO task metadata and Vizier route decisions, keeping Concho creation deferred until ER-0066.
- Alternatives considered: merging task visualization into ER-0066 was rejected because observer-driven session growth and task-specific view modeling have separate acceptance surfaces.

## Acceptance Criteria

- Tests verify route decisions for known task-state relationships.
- Tests verify task visualization objects preserve task identity, state, artifacts, and diagnostics.
- Tests verify unknown or incomplete task metadata is ignored deterministically.
- Existing artifact routing tests continue to pass.

## Risks / Open Questions

- Risk: the first Task Concho metadata model may need revision when richer task lifecycle states are introduced.
- Question: which CEO task states should be considered routable in the first implementation?

## Dependencies

- Dependency 1: AR-0025 Conch and Vizier Interaction Modes.
- Dependency 2: ER-0063 Referee Graph Watch and Change-Feed API.
- Dependency 3: ER-0064 Vizier Relationship-Pattern Routing.

## Implementation Notes

- Notes for implementer: keep route decisions separate from session mutation.
- Notes for implementer: if Autotools inputs change, regenerate only when necessary and document the command.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create task-state relationships and verify Vizier returns stable task route decisions.
