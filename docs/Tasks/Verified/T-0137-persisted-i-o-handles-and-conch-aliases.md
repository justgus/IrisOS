---
Legacy-ID: ER-0045.4
GitHub-Issue: #181
Source-Path: docs/ER/ER-0045.4-Persisted-IO-Handles-and-Conch-Aliases.md
---

# T-0137 — Persisted I/O Handles and Conch Aliases

## Task Metadata

- Task ID: T-0137
- Legacy ID: ER-0045.4
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #181
- AR Dependencies: -
- Date Requested: 2026-03-07
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0045.4-Persisted-IO-Handles-and-Conch-Aliases.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #181
---

# ER-0045.4 — Persisted I/O Handles and Conch Aliases

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0045.4
- Title: Persisted I/O Handles and Conch Aliases
- Status: Verified
- Date: 2026-03-07
- Owners: Mike
- Type: Enhancement

## Context

- Conch I/O handles are currently session-only and ephemeral.
- Kernel demo workflows need stable, aliasable handle references without cross-process/global scope.

## Goals

- Introduce persisted I/O handle objects managed by Conch aliases.
- Allow handles to be referenced via Conch alias names across a single Conch session.
- Ensure handle cleanup is deterministic and explicit.

## Non-Goals

- Cross-process or global handle lookup.
- OS-level resource persistence across restarts.

## Scope

- In scope: persisted handle metadata objects in Referee and Conch alias integration.
- In scope: Conch commands for saving/loading handle aliases.
- Out of scope: multi-user handle sharing or daemonized handle registry.

## Requirements

- Functional: Conch can persist handle references in the Referee store via aliases.
- Functional: Conch can resolve a persisted alias to a handle within the same session.
- Non-functional: error messages are explicit for stale or missing handle references.

## Proposed Approach

- Define a minimal schema type for persisted I/O handle metadata.
- Extend Conch alias handling to support handle references alongside ObjectIDs.
- Implement explicit `io alias`/`io unalias` (names TBD) if needed for clarity.

## Acceptance Criteria

- Persisted I/O handle aliases can be created, listed, and resolved in Conch.
- Invalid or stale aliases produce clear errors.
- Tests cover persisted alias creation, lookup, and cleanup.

## Dependencies

- ER-0045.2 Conch Commands and Invocation Verification.
- ER-0045.3 End-to-End I/O Integration Tests.

## Verification Plan

- Tests: Conch alias tests and handle persistence tests in `make check`.
- Manual: create an alias for a handle and resolve it in the same session.
```
