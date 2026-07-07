---
Legacy-ID: ER-0063
GitHub-Issue: #270
Source-Path: docs/ER/ER-0063-Referee-Graph-Watch-and-Change-Feed-API.md
---

# T-0159 — Referee Graph Watch and Change-Feed API

## Task Metadata

- Task ID: T-0159
- Legacy ID: ER-0063
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #270
- AR Dependencies: AR-0009, AR-0011, AR-0012, AR-0025
- Date Requested: 2026-05-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0063-Referee-Graph-Watch-and-Change-Feed-API.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #270
AR-Dependencies: AR-0009, AR-0011, AR-0012, AR-0025
---

# ER-0063 — Referee Graph Watch and Change-Feed API

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0063
- Title: Referee Graph Watch and Change-Feed API
- Status: Verified
- Date: 2026-05-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Conch and Vizier can query existing Referee relationships, but they do not yet have a stable API for observing graph changes or replaying relationship changes as a feed.
- Background / constraints: AR-0025 Mode 2 calls for observer-driven graph routing. ER-0009 implemented routed artifact behavior, and ER-0010 defined Viz artifact relationships, but follow-on work needs a deterministic Referee-level change surface before Conch session growth can become observer-driven.

## Goals

- Define a first Referee graph watch or change-feed API over object and relationship changes.
- Support deterministic snapshot-and-replay behavior for relationship-driven consumers such as Vizier.
- Allow consumers to filter graph changes by edge name, edge role, source object, target object, and object type where existing metadata supports it.
- Preserve existing object and edge persistence behavior.

## Non-Goals

- Full asynchronous event loop integration.
- Distributed subscriptions or multi-process notification transport.
- Conch session lifecycle changes.
- Vizier relationship-pattern routing implementation.
- Task Concho or task-state visualization.
- Authorization, sandbox, or capability enforcement for graph subscriptions.

## Scope

- In scope: an in-process Referee API for enumerating graph changes from a deterministic cursor or snapshot point.
- In scope: change records for object creation and edge creation.
- In scope: filter structures for relationship patterns needed by downstream Vizier work.
- In scope: tests for deterministic ordering, filtering, and persistence/reload behavior.
- Out of scope: delete events, mutable object updates, remote watchers, and UI spawning behavior.

## Requirements

- Functional: callers can obtain a stable cursor or snapshot marker for the current graph state.
- Functional: callers can list graph changes after a cursor in deterministic order.
- Functional: callers can filter edge changes by name and role.
- Functional: callers can filter object changes by type.
- Functional: persisted stores preserve enough ordering metadata for replay after reopen.
- Non-functional: existing `create_object`, `add_edge`, `edges_from`, and `edges_to` behavior remains unchanged.
- Non-functional: the API remains usable in tests without threads or background processes.

## Proposed Approach

- Summary: add a small Referee change-feed abstraction around the existing append-only object and edge segment behavior, exposing typed change records and simple filters. Keep the first implementation pull-based so ER-0064 and later Conch work can consume snapshots without requiring an event loop.
- Alternatives considered: a callback-based watcher was deferred because the service/event-loop ownership model is not defined yet. Polling raw edge indexes was rejected because Vizier and Conch should not depend on storage internals.

## Acceptance Criteria

- Tests verify object creation appears in the graph change feed.
- Tests verify edge creation appears in the graph change feed.
- Tests verify edge name and role filters return only matching relationship changes.
- Tests verify object type filters return only matching object changes.
- Tests verify a cursor taken before changes can replay those changes after store reopen.
- Existing Referee edge query tests continue to pass.

## Risks / Open Questions

- Risk: cursor semantics may need revision when object versioning becomes broader than create-only records.
- Risk: exposing too much storage ordering detail could constrain later segment/index changes.
- Question: should the initial cursor be a storage sequence number, a timestamp boundary, or an opaque token?
- Question: should live callback delivery be a follow-on ER or part of a later service-plane observer API?

## Dependencies

- Dependency 1: AR-0009 Referee Object Store.
- Dependency 2: AR-0011 Vizier Interpretation Layer.
- Dependency 3: AR-0012 Conch Shell and Conchos.
- Dependency 4: AR-0025 Conch and Vizier Interaction Modes.
- Dependency 5: ER-0003 Phase1 Referee Graph Enhancements.
- Dependency 6: ER-0009 Phase3 Vizier Routing.
- Dependency 7: ER-0010 Phase3 Viz Artifacts.

## Implementation Notes

- Notes for implementer: prefer an opaque cursor type unless the System Engineer approves a concrete sequence-number contract.
- Notes for implementer: keep this API pull-based and in-process for the first slice.
- Notes for implementer: do not introduce threads, async runtimes, or new dependencies for ER-0063.
- Notes for implementer: if Autotools inputs change, regenerate only when necessary and document the command.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create objects and edges, take a cursor, add more relationships, then replay filtered changes from that cursor.
  - reopen the store and confirm replay order and filters remain deterministic.
```
