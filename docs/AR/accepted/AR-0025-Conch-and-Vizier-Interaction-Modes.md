---
GitHub-Issue: #293
ER-Dependencies: ER-0008, ER-0009, ER-0010, ER-0011, ER-0052, ER-0054, ER-0063, ER-0064, ER-0065, ER-0066, ER-0084
---

# AR-0025 — Conch And Vizier Interaction Modes

- Status: Implemented
- Date: 2026-03-14
- Owners: Mike

## Context

AR-0011 and AR-0012 describe a graph-driven interaction model in which Conch subscribes to object
graph changes and Vizier maps those changes into Concho views. The current implementation already
supports deterministic artifact routing and explicit Concho spawning, but it does not yet implement
a general graph-subscription pipeline or Task Concho routing.

## Recommendation

Define explicit interaction modes for Conch and Vizier.

### Mode 1: Triggered Artifact Routing

- Artifact creation or command paths trigger Vizier routing
- Vizier resolves route by type or preferred renderer metadata
- Conch spawns a Concho explicitly for the routed artifact

### Mode 2: Observer-Driven Graph Routing

- Conch session subscribes to graph changes or graph snapshots
- Vizier evaluates relationship patterns, task state, and artifact type
- Concho creation is observer-driven rather than explicitly invoked by command handlers

### Task Visualization

- Task visualization is part of Mode 2 unless an explicit Task view model is accepted sooner

## Goals

- Recognize the existing routed-artifact behavior as a valid architecture stage.
- Define a clean path to the fuller observer-driven UI model.
- Avoid treating command-triggered routing as an undocumented shortcut.

## Non-Goals (v1)

- Full compositor or layout-manager design
- Multi-user collaborative session semantics
- A pluggable renderer ecosystem

## Relationship To Existing ARs

- Refines [AR-0011-Vizier-Interpretation-Layer.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md)
- Refines [AR-0012-Conch-Shell-and-Conchos.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0012-Conch-Shell-and-Conchos.md)
- Aligns with [AR-0013-Viz-Display-Subsystem.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0013-Viz-Display-Subsystem.md)

## Next Steps

- Continuous event-driven notification is a future extension; the accepted snapshot/change-feed
  form of Mode 2 is implemented.

## Implementation Progress

- Mode 1 is implemented by artifact routing and explicit Concho creation in `src/vizier/routing.cc`
  and `src/conch_shell/conch.cc`.
- ER-0063 provides the persistent cursor-based graph change feed. ER-0064 routes relationships,
  ER-0065 provides task views, and ER-0066 materializes observer-driven session growth in
  `src/conch/session_growth.cc`. ER-0084 invokes session updates at shell command boundaries.
- All listed ER dependencies are Verified. The 2026-10-13 reconciliation source audit confirms
  both modes and Task Conchos; fresh `TMPDIR=/private/tmp make check` passed 31/31 outside the
  sandbox, including routing, session-growth, and Conch integration suites.
- Mode 2 uses graph changes/snapshots at command boundaries. This status does not claim continuous
  notification while the shell is idle.
