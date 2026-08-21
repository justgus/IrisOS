---
Legacy-ID: ER-0036
GitHub-Issue: #172
Source-Path: docs/ER/ER-0036-Definition-Versioning-and-Migration-Hooks.md
---

# T-0124 — Definition Versioning and Migration Hooks

## Task Metadata

- Task ID: T-0124
- Legacy ID: ER-0036
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #172
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0036-Definition-Versioning-and-Migration-Hooks.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #172
---

# ER-0036 — Definition Versioning and Migration Hooks

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0036
- Title: Definition Versioning and Migration Hooks
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v1 persistence must support Definition versioning with explicit migration hooks.
- Background / constraints: Refract Definitions must supersede prior versions deterministically.

## Goals

- Define versioning metadata and supersedes chains for Definitions.
- Provide migration hook metadata for schema evolution.

## Non-Goals

- Full automated migration engine with complex transforms.
- Cross-store migration tooling.

## Scope

- In scope: Definition version fields, supersedes links, migration hook metadata.
- Out of scope: complete migration execution framework.

## Requirements

- Functional: Definitions can declare supersedes chains and optional migration hooks.
- Non-functional: deterministic version ordering and lookup.

## Proposed Approach

- Summary: extend Refract Definition metadata to include version and migration hooks.
- Alternatives considered: implicit versioning (rejected).

## Acceptance Criteria

- Definitions list their supersedes chain and migration hooks.
- Referee can resolve latest Definition for a Class.

## Risks / Open Questions

- Risk: inconsistent versioning if definitions are edited in place.
- Question: what minimum migration hook interface is needed for v1?

## Dependencies

- Dependency 1: ER-0034 Referee Storage Layout Implementation (v1).

## Implementation Notes

- Notes for implementer: keep hooks declarative and minimal.

## Verification Plan

- Tests to run: version chain resolution tests.
- Manual checks: migrate a sample Definition v1 -> v2 and validate lookup.
```
