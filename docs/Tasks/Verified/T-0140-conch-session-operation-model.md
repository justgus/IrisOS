---
Legacy-ID: ER-0047.1
GitHub-Issue: #182
Source-Path: docs/ER/ER-0047.1-Conch-Session-Operation-Model.md
---

# T-0140 — Conch Session Operation Model

## Task Metadata

- Task ID: T-0140
- Legacy ID: ER-0047.1
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #182
- AR Dependencies: -
- Date Requested: 2026-03-07
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0047.1-Conch-Session-Operation-Model.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #182
---

# ER-0047.1 — Conch Session Operation Model

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0047.1
- Title: Conch Session Operation Model
- Status: Verified
- Date: 2026-03-07
- Owners: Mike
- Type: Enhancement

## Context

- Conch commands are currently built into the shell.
- We now support object instance operations and want Conch commands to align with Conduit.

## Goals

- Define `Conch::Session` as the primary command surface.
- Represent commands as operations on the session object.
- Provide alias bindings for human-friendly command names.

## Non-Goals

- Full migration of all commands (handled in later sub-ERs).
- Cross-session command registration.

## Scope

- In scope: schema definitions for session operations and alias mapping strategy.
- In scope: initial dispatcher design for Conch command → operation resolution.
- Out of scope: implementation of all command operations.

## Requirements

- Functional: session operations are discoverable in `show type Conch::Session`.
- Functional: alias map can route CLI commands to session operations.
- Non-functional: clear separation between CLI parsing and operation dispatch.

## Proposed Approach

- Add operation metadata for session commands in the schema bootstrap.
- Introduce a dispatcher layer that maps command tokens to operation names via aliases.
- Keep existing command code paths intact until migrated.

## Acceptance Criteria

- `Conch::Session` exposes a defined set of operations in the schema.
- Aliases map CLI commands to those operations.
- Dispatcher can resolve a command to a session operation without executing it.

## Dependencies

- ER-0030 Conduit Conch Integration.
- ER-0042 Core Operations Metadata and Bindings.

## Verification Plan

- Tests: schema bootstrap tests and alias resolution tests.
```
