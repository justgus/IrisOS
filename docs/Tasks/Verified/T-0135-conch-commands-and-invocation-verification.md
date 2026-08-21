---
Legacy-ID: ER-0045.2
GitHub-Issue: #181
Source-Path: docs/ER/ER-0045.2-Conch-Commands-and-Invocation-Verification.md
---

# T-0135 — Conch Commands and Invocation Verification

## Task Metadata

- Task ID: T-0135
- Legacy ID: ER-0045.2
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #181
- AR Dependencies: -
- Date Requested: 2026-03-07
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0045.2-Conch-Commands-and-Invocation-Verification.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #181
---

# ER-0045.2 — Conch Commands and Invocation Verification

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0045.2
- Title: Conch Commands and Invocation Verification
- Status: Verified
- Date: 2026-03-07
- Owners: Mike
- Type: Enhancement

## Context

- Kernel I/O primitives must be invocable from Conch with correct validation.
- Existing Conch commands only cover a small subset of core operations.

## Goals

- Provide Conch commands that exercise kernel I/O operations end-to-end.
- Verify invocation parameters and operation resolution before execution.
- Enforce capability checks on I/O operations.

## Non-Goals

- Interactive TUI/GUI for I/O inspection.
- Long-lived background I/O jobs beyond current v1 scope.

## Scope

- In scope: Conch commands for open/close, send/recv, and await_readable.
- In scope: argument validation, dispatch verification, capability checks.
- Out of scope: device driver commands or advanced networking configuration.

## Requirements

- Functional: Conch supports `io open`, `io close`, `io send`, `io recv`, `io await`, and `io handles`.
- Functional: Conch supports `task spawn` and `task list` for CEO task IDs.
- Functional: invalid arguments and dispatch failures are reported clearly.
- Non-functional: commands are deterministic and testable.

## Proposed Approach

- Extend Conch command routing with a dedicated I/O command group.
- Validate task IDs, handle names, and payload encodings before invocation.
- Use Conduit dispatch for operation lookup and execution, enforcing required capabilities.

## Acceptance Criteria

- Conch can open a channel/datagram port between two tasks and close it cleanly.
- Conch can send bytes, await readability, and receive bytes over the same handle.
- Conch verifies invocation arguments and reports errors without crashing.
- Conch tests cover both success and failure scenarios.

## Dependencies

- ER-0045.1 Conduit I/O Operations and Schema Definitions.
- ER-0044 Capability Hooks and Policy Plumbing.

## Verification Plan

- Tests: Conch command tests for open/send/await/recv/close.
- Manual: run a Conch session script that exercises each I/O command end-to-end.
```
