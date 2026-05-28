---
GitHub-Issue: N/A
AR-Dependencies: AR-0011, AR-0012, AR-0013, AR-0025
ER-Dependencies: ER-0008, ER-0009, ER-0010, ER-0063, ER-0064, ER-0065
---

# ER-0066 — Observer-Driven Conch Session Growth

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0066
- Title: Observer-Driven Conch Session Growth
- Status: Proposed
- Date: 2026-05-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Conch can display routed artifacts through explicit command/helper paths, but Conch sessions do not yet grow from observed graph changes as described by AR-0012 and AR-0025 Mode 2.
- Background / constraints: ER-0063 added a pull-based Referee graph change feed, ER-0064 added artifact relationship-route decisions, and ER-0065 defines task visualization route decisions. Conch still needs a deterministic session update layer that consumes those decisions and creates Concho view objects without command handlers explicitly invoking artifact spawning.

## Goals

- Add observer-driven Conch session growth from Referee graph changes.
- Consume ER-0063 graph cursors and graph-change records through a deterministic session update API.
- Use ER-0064 Vizier route decisions to create or link Concho views for routed artifact relationships.
- Avoid duplicate Conchos for the same session/artifact/relationship route.
- Preserve existing command-triggered routing behavior during the transition.

## Non-Goals

- Full asynchronous event loop implementation.
- Multi-user collaborative session semantics.
- Layout manager, compositor, focus, or tiling policy.
- Renderer implementation or visual styling.
- New task visualization object definitions beyond ER-0065.
- Authorization, capability, or sandbox enforcement for session observation.

## Scope

- In scope: a Conch session growth/update surface that advances from a graph cursor.
- In scope: persisted or inspectable session state needed to remember the last consumed cursor.
- In scope: deterministic Concho creation or session-to-Concho linkage from Vizier relationship-route decisions, including task visualization routes defined by ER-0065.
- In scope: duplicate suppression for repeated updates over the same graph changes.
- In scope: tests that create graph relationships, run a session update, and verify resulting Concho/session graph state.
- Out of scope: long-running watcher threads, blocking subscription APIs, remote event delivery, and UI layout behavior.

## Requirements

- Functional: a Conch session can store or receive a graph cursor and request updates after that cursor.
- Functional: routed artifact relationships observed after the cursor create the expected Concho/session graph representation.
- Functional: repeated updates over the same cursor range do not create duplicate Conchos.
- Functional: unknown graph changes and unrouted relationships are ignored without error.
- Functional: explicit command-triggered `spawn_concho_for_artifact` behavior remains available.
- Functional: update results report how many changes were examined and how many Conchos were created or reused.
- Non-functional: the first implementation remains in-process, pull-based, and testable without threads.

## Proposed Approach

- Summary: add a small Conch session observer/update component that pulls graph changes from Referee, asks Vizier for relationship-route decisions, and materializes Concho/session links for new routed artifacts. Keep the API explicit and cursor-driven so tests can drive session growth deterministically before any live event-loop design is accepted.
- Alternatives considered: adding a background watcher directly to Conch was deferred because ownership, lifecycle, and scheduling semantics are not defined yet. Leaving graph growth in command handlers was rejected because it does not satisfy AR-0012's subscription model.

## Acceptance Criteria

- Tests verify a Conch session update creates a Concho for a newly observed `produced` artifact relationship.
- Tests verify `progress`, `diagnostic`, or `stream` routed relationships can drive the same session growth path.
- Tests verify task visualization route decisions from ER-0065 can drive Task Concho session growth.
- Tests verify repeated updates do not create duplicate Conchos for the same routed relationship.
- Tests verify unrouted graph changes are ignored.
- Tests verify update results expose consumed cursor and created/reused counts.
- Existing explicit Vizier artifact spawning tests continue to pass.

## Risks / Open Questions

- Risk: the first duplicate-suppression key may need revision once layout/session identity semantics become richer.
- Risk: session cursor persistence may need migration if graph cursors become distributed or durable across store compaction.
- Question: should Conch session state be a first-class Refract type in this ER or use the existing `Conch::Concho` object model until a broader session schema is accepted?
- Question: what duplicate-suppression key should Task Conchos use if a task has multiple routed artifacts and diagnostics?

## Dependencies

- Dependency 1: AR-0011 Vizier Interpretation Layer.
- Dependency 2: AR-0012 Conch Shell and Conchos.
- Dependency 3: AR-0013 Viz Display Subsystem.
- Dependency 4: AR-0025 Conch and Vizier Interaction Modes.
- Dependency 5: ER-0008 Phase3 Conch Shell.
- Dependency 6: ER-0009 Phase3 Vizier Routing.
- Dependency 7: ER-0010 Phase3 Viz Artifacts.
- Dependency 8: ER-0063 Referee Graph Watch and Change-Feed API.
- Dependency 9: ER-0064 Vizier Relationship-Pattern Routing.
- Dependency 10: ER-0065 Task Visualization Objects and Task Conchos.

## Implementation Notes

- Notes for implementer: keep the first session observer pull-based and deterministic.
- Notes for implementer: do not introduce threads, async runtimes, or new dependencies for ER-0066.
- Notes for implementer: keep Concho creation/linking separate from layout policy.
- Notes for implementer: consume task visualization route decisions from ER-0065; do not invent new task view semantics in this ER.
- Notes for implementer: if Autotools inputs change, regenerate only when necessary and document the command.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create a Conch session, create a producer-to-artifact relationship, run the session update, and inspect the resulting Concho/session graph state.
  - rerun the same update path and confirm no duplicate Concho is created.
