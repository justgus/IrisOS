---
Legacy-ID: ER-0064
GitHub-Issue: #271
Source-Path: docs/ER/ER-0064-Vizier-Relationship-Pattern-Routing.md
---

# T-0160 — Vizier Relationship-Pattern Routing

## Task Metadata

- Task ID: T-0160
- Legacy ID: ER-0064
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #271
- AR Dependencies: AR-0011, AR-0012, AR-0013, AR-0025
- Date Requested: 2026-05-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0064-Vizier-Relationship-Pattern-Routing.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #271
AR-Dependencies: AR-0011, AR-0012, AR-0013, AR-0025
ER-Dependencies: ER-0009, ER-0010, ER-0063
---

# ER-0064 — Vizier Relationship-Pattern Routing

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0064
- Title: Vizier Relationship-Pattern Routing
- Status: Verified
- Date: 2026-05-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Vizier can route individual artifacts by type or preferred renderer, but it does not yet evaluate Referee relationship patterns such as `produced`, `progress`, `diagnostic`, or `stream` as routing inputs.
- Background / constraints: AR-0011 assigns relationship-pattern interpretation to Vizier, and AR-0025 Mode 2 requires Vizier to evaluate relationship patterns, task state, and artifact type. ER-0063 defines the Referee graph watch/change-feed surface that this ER should consume.

## Goals

- Add a Vizier routing surface for relationship changes and graph snapshots.
- Route known artifact relationships using the target object type and existing preferred-renderer metadata.
- Preserve existing explicit artifact routing behavior from ER-0009.
- Make relationship-pattern routing deterministic and testable without Conch UI code.

## Non-Goals

- Referee graph change-feed implementation.
- Conch session subscription or Concho lifecycle management.
- Task-state routing and Task Conchos.
- Layout, focus, compositor, or renderer behavior.
- Pluggable renderer policy.
- Authorization or capability checks for graph observation.

## Scope

- In scope: relationship-pattern route decisions for artifact-oriented edge names such as `produced`, `progress`, `diagnostic`, and `stream`.
- In scope: deterministic handling for unknown relationship names, unknown target types, and missing preferred-renderer metadata.
- In scope: APIs that accept Referee edge/change records or equivalent snapshot inputs and return route decisions.
- In scope: tests that exercise relationship routing without spawning UI.
- Out of scope: creating Concho objects from graph feed events; that belongs to observer-driven Conch session growth.

## Requirements

- Functional: Vizier can route a `produced` artifact relationship to the same Concho route as direct artifact routing.
- Functional: Vizier can route `progress`, `diagnostic`, and `stream` artifact relationships when the target artifact type has a known route.
- Functional: preferred renderer metadata remains the first routing source for the target artifact type.
- Functional: unknown relationships or unknown target types produce no route rather than an error.
- Functional: route decisions include enough context for later Conch session growth to link the source object, artifact object, relationship name, and selected Concho route.
- Non-functional: existing `route_for_type`, `route_for_type_id`, and `spawn_concho_for_artifact` behavior remains unchanged unless the System Engineer approves a broader API change.

## Proposed Approach

- Summary: extend Vizier routing with a small relationship-route decision model that consumes Referee relationship data and resolves target artifact type through the Refract schema registry. Keep the implementation pure and pull-based so ER-0064 can be tested independently of live graph watchers and Conch session mutation.
- Alternatives considered: embedding relationship routing directly in Conch was rejected because AR-0011 places interpretation in Vizier. Treating every relationship as routable was rejected because non-artifact graph edges should not cause UI growth by default.

## Acceptance Criteria

- Tests verify `produced` relationships route known Viz artifact target types.
- Tests verify `progress`, `diagnostic`, and `stream` relationships route known Viz artifact target types.
- Tests verify preferred renderer metadata is honored for relationship target types.
- Tests verify unknown relationship names do not route.
- Tests verify unknown target artifact types do not route.
- Existing Vizier direct artifact routing tests continue to pass.

## Risks / Open Questions

- Risk: the initial list of relationship names may be too narrow for future producers.
- Risk: route decisions may need richer policy metadata once task routing and session growth are implemented.
- Question: should `summary` and `summarizes` graph relationships be routable in this ER or deferred to a composite-view ER?
- Question: should relationship routing return a single route or allow multiple Concho candidates?

## Dependencies

- Dependency 1: AR-0011 Vizier Interpretation Layer.
- Dependency 2: AR-0012 Conch Shell and Conchos.
- Dependency 3: AR-0013 Viz Display Subsystem.
- Dependency 4: AR-0025 Conch and Vizier Interaction Modes.
- Dependency 5: ER-0009 Phase3 Vizier Routing.
- Dependency 6: ER-0010 Phase3 Viz Artifacts.
- Dependency 7: ER-0063 Referee Graph Watch and Change-Feed API.

## Implementation Notes

- Notes for implementer: keep relationship routing separate from Concho creation so tests can assert route decisions directly.
- Notes for implementer: prefer existing `route_for_type` and preferred-renderer behavior rather than duplicating type-routing tables.
- Notes for implementer: do not implement task-state routing in ER-0064; reserve it for ER-0065 unless the System Engineer changes scope.
- Notes for implementer: if Autotools inputs change, regenerate only when necessary and document the command.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create a source object and a Viz artifact, link them with `produced`, and verify Vizier returns the expected route decision.
  - repeat with `progress`, `diagnostic`, and `stream` relationships.
```
