---
GitHub-Issue: #184
---

# ER-0054 — Conch Namespace Navigation

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0054
- Title: Conch Namespace Navigation
- Status: Proposed
- Date: 2026-03-08
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: `ls` output is too verbose as the schema grows.
- Background / constraints: Conch needs a namespace-scoped view of schemas, similar to directories and files in a shell.

## Goals

- Provide namespace-scoped listing and navigation.
- Make `ls` default to namespace-level output instead of full details.
- Preserve access to the full `ls` output via flags.

## Non-Goals

- Changing object or schema storage formats.
- Introducing new schema definitions beyond navigation state and output control.

## Scope

- In scope: namespace navigation command (`namespace` and `ns` aliases).
- In scope: prompt reflects current namespace.
- In scope: `ls` updated to show namespace-level output by default.
- In scope: recursive and object listing flags as described below.
- Out of scope: new utilities beyond navigation/listing.

## Requirements

- Functional: `ls` at root lists only root namespaces by default.
- Functional: `namespace <name>` / `ns <name>` changes current namespace.
- Functional: prompt reflects current namespace.
- Functional: `ls` at a namespace shows types and namespaces at that level.
- Functional: `ls --recursive` lists namespaces down the tree without object references.
- Functional: `ls --recursive --objects` matches current `ls` output (full objects at all levels).
- Functional: `ls --objects` includes object references for types at the current level.
- Non-functional: output remains deterministic and stable.

## Proposed Approach

- Track a current namespace path in the Conch session state.
- Update `ls` to use the current namespace as its root unless overridden.
- Add `namespace`/`ns` command to change the current namespace (supports root reset).
- Add flags to `ls` to control recursion and object listing.

## Acceptance Criteria

- `ls` defaults to namespace-only listing at the current scope.
- `namespace`/`ns` changes the prompt and filters `ls` output accordingly.
- `ls --recursive` and `ls --objects` behave as specified above.

## Dependencies

- ER-0047.x Conch session operation model and command migration (for dispatch consistency).

## Verification Plan

- Tests: Conch listing regression tests for namespace scoping and flags.
- Manual checks:
  - `ls` at root shows namespaces only.
  - `namespace <ns>` updates prompt and `ls` output.
  - `ls --objects` includes object references for types at current scope.
  - `ls --recursive` and `ls --recursive --objects` match expected outputs.
