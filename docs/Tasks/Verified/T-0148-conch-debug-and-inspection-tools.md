---
Legacy-ID: ER-0052
GitHub-Issue: #188
Source-Path: docs/ER/ER-0052-Conch-Debug-and-Inspection-Tools.md
---

# T-0148 — Conch Debug and Inspection Tools

## Task Metadata

- Task ID: T-0148
- Legacy ID: ER-0052
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #188
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0052-Conch-Debug-and-Inspection-Tools.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #188
---

# ER-0052 — Conch Debug and Inspection Tools

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0052
- Title: Conch Debug and Inspection Tools
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v2 needs tooling to inspect and debug object graphs, schemas, and operations.
- Background / constraints: Conch is the primary userland interface.

## Goals

- Add debugging commands for object graphs, relationships, and operation dispatch.
- Provide inspection commands for schema versions and migrations.

## Non-Goals

- Full interactive debugger for task execution.
- GUI tooling beyond Conch.

## Scope

- In scope: Conch commands for inspection and debugging.
- Out of scope: advanced scripting or GUI-based inspection.

## Requirements

- Functional: users can inspect object relationships and operation dispatch decisions.
- Non-functional: stable output formats for tooling.

## Proposed Approach

- Summary: add debug-focused Conch commands and expose Conduit diagnostics.
- Alternatives considered: separate external tooling only (rejected).

## Acceptance Criteria

- Conch can display operation resolution traces.
- Object graph inspection provides relationship diagnostics.

## Risks / Open Questions

- Risk: debug outputs may overwhelm standard usage.
- Question: which debug commands are essential for v2?

## Dependencies

- Dependency 1: ER-0030 Conduit Conch Integration.
- Dependency 2: ER-0044 Capability Hooks and Policy Plumbing.

## Implementation Notes

- Notes for implementer: keep debug outputs optional and clearly labeled.

## Verification Plan

- Tests to run: command output tests for debug commands.
- Manual checks: run debug commands on sample object graphs.
```
