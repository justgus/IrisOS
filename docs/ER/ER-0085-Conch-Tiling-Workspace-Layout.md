---
GitHub-Issue: N/A
AR-Dependencies: AR-0012
ER-Dependencies: ER-0084
---

# ER-0085 — Conch Tiling Workspace Layout

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass; the human records the required live pass.

## ER Metadata

- ER ID: ER-0085
- Title: Conch Tiling Workspace Layout
- Status: In Progress
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0012 recommends that the Conch workspace tile as Conchos appear and allows Conchos to own nested sub-Conchos. The current verified session-growth work creates graph objects but explicitly leaves layout behavior out of scope.
- Background / constraints: This ER follows the shell integration in ER-0084. It introduces a deterministic object-backed layout model and textual inspection; it does not add a graphical compositor.

## Goals

- Represent the active session's workspace as a deterministic tiling tree of Conchos.
- Add newly observed Conchos to the workspace and represent nested Conchos under their owning Concho.
- Make the layout inspectable and persistent through Referee objects and relationships.

## Non-Goals

- Render graphics, manage pixel geometry, focus, input, or window decorations.
- Add drag-and-drop, user-configurable split direction, or resize behavior.
- Change Vizier routing or session graph observation.

## Scope

- In scope: a Refract schema for workspace/tile state, deterministic insertion of session Conchos, nested ownership representation, shell inspection output, and tests.
- In scope: add `test_conch_layout` to `TESTS` and `check_PROGRAMS` in `tests/Makefile.am` with its source and link rule, so both the focused command and `make check` run the layout tests.
- Out of scope: compositor and rendering systems, remote synchronization, and interaction modes belonging only to AR-0025.

## Requirements

- Functional: each active Conch session has a root workspace tile representation.
- Functional: different Conch sessions have different workspace roots and tile objects; a new session starts with no tiles from earlier sessions.
- Functional: when opening a legacy session whose workspace payload belongs to a different session, create and use a session-owned workspace instead of continuing to share it.
- Functional: newly linked Conchos are added exactly once in stable graph-observation order.
- Functional: a Concho with child Conchos is represented as a nested tile container with stable child order.
- Functional: reopening the store reconstructs the same tile tree from persisted objects and relationships.
- Non-functional: layout remains a data model that can be tested without a graphics device or external dependency.

## Proposed Approach

- Summary: define a small recursive tile-tree object model, persist session tile membership and nested parent/child relationships through Referee, and update the tree from the active session's Concho links. Workspace membership edges index all of that session's tiles; tile containment edges represent nesting. Reconcile the displayed parent from current Concho ownership so a child linked before its owner can move under that owner when the relationship appears. Preserve graph-observation order and expose the tree through the `workspace` command.
- Alternatives considered: pixel geometry and a renderer were deferred because AR-0012 requires an expanding tiling workspace but does not require a graphical compositor in this slice.

## Acceptance Criteria

- Tests verify every session gets its own workspace root and a new session starts without earlier session tiles; a legacy shared workspace link is repaired idempotently.
- Tests verify an observed Concho is inserted once in that session's workspace.
- Tests verify repeated updates preserve stable tile order and do not duplicate tiles.
- Tests verify nested Concho ownership appears as a nested tile subtree.
- Tests verify reopening the store preserves the same tree when inspecting the original persisted session, while creating a new session yields an independent workspace.
- An independent live pass confirms the shell displays the growing and nested tile tree in a readable form.

## Risks / Open Questions

- Risk: the deterministic first split policy may need to evolve when a graphical renderer is introduced.
- Decision (System Engineer, 2026-10-09): each newly created shell session gets a separate workspace root and its own tiles. A new session starts empty; an existing session's workspace remains persisted and is independently reconstructable from its session object. Sessions do not share a workspace. When an old session edge points at a workspace payload owned by another session, use a new session-owned workspace; the append-only store retains the historical edge but does not treat it as the active workspace.

## Dependencies

- Dependency 1: ER-0084 Conch Shell Session-Growth Integration.
- Dependency 2: AR-0012 Conch Shell and Conchos.

## Implementation Notes

- Do not invent pixel dimensions or graphics behavior.
- Preserve stable insertion order across reopen.
- Keep the tree model separable from a future renderer.
- Keep workspace membership distinct from tile containment: the object graph is append-only, while the displayed hierarchy follows the selected current parent for each session tile.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_conch_layout test_conch_authoring`
  - `make check`
- Manual checks: run `workspace` to inspect a session with multiple routed Conchos and a nested Concho in the textual workspace tree. Restart Conch with the same database and verify the new session reports a distinct ID and an empty workspace; automated persistence coverage verifies the prior session's tree remains stored independently.

### Independent Live Pass

- Required: Yes
- If no, reason: N/A
- Build / environment / target: Built `bin/conch` in a terminal with an interactive TTY.
- Steps: Start Conch with a persistent database; create routed artifacts and a nested Concho; inspect the workspace tree; exit and reopen the same database; inspect the new session's workspace.
- Expected observations: newly created views appear as stable tiles and nested views appear beneath their owner; the new session has a distinct identity, a distinct empty workspace, and no shared tiles from the earlier session.
- Observed results: Pending human test.
- Result: Pending
- Performed by: Pending
- Date: Pending
