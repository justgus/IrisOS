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
- Status: Approved
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
- Functional: newly linked Conchos are added exactly once in stable graph-observation order.
- Functional: a Concho with child Conchos is represented as a nested tile container with stable child order.
- Functional: reopening the store reconstructs the same tile tree from persisted objects and relationships.
- Non-functional: layout remains a data model that can be tested without a graphics device or external dependency.

## Proposed Approach

- Summary: define a small recursive tile-tree object model, persist tile membership and parent/child relationships through Referee, and update the tree from the active session's Concho links. Use a documented deterministic split rule for insertion, chosen to keep the first implementation simple and reproducible. Expose a textual tree inspection command.
- Alternatives considered: pixel geometry and a renderer were deferred because AR-0012 requires an expanding tiling workspace but does not require a graphical compositor in this slice.

## Acceptance Criteria

- Tests verify the workspace root is created for a session and an observed Concho is inserted once.
- Tests verify repeated updates preserve stable tile order and do not duplicate tiles.
- Tests verify nested Concho ownership appears as a nested tile subtree.
- Tests verify persistence/reopen preserves the same tree.
- A human live pass confirms the shell displays the growing and nested tile tree in a readable form.

## Risks / Open Questions

- Risk: the deterministic first split policy may need to evolve when a graphical renderer is introduced.
- Question: none; the first layout is structural and deterministic, leaving visual geometry to a future accepted design.

## Dependencies

- Dependency 1: ER-0084 Conch Shell Session-Growth Integration.
- Dependency 2: AR-0012 Conch Shell and Conchos.

## Implementation Notes

- Do not invent pixel dimensions or graphics behavior.
- Preserve stable insertion order across reopen.
- Keep the tree model separable from a future renderer.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_conch_layout test_conch_authoring`
  - `make check`
- Manual checks: inspect a session with multiple routed Conchos and a nested Concho in the textual workspace tree.

### Live Pass (Human)

- Required: Yes
- If no, reason: N/A
- Build / environment / target: Built `bin/conch` in a terminal with an interactive TTY.
- Steps: Start Conch with a persistent database; create routed artifacts and a nested Concho; inspect the workspace tree; exit and reopen the same database; inspect again.
- Expected observations: newly created views appear as stable tiles, nested views appear beneath their owner, and the same tree appears after reopening.
- Observed results: Pending human test.
- Result: Pending
- Performed by: Pending
- Date: Pending
