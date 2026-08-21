---
Legacy-ID: ER-0044
GitHub-Issue: #180
Source-Path: docs/ER/ER-0044-Capability-Hooks-and-Policy-Plumbing.md
---

# T-0132 — Capability Hooks and Policy Plumbing

## Task Metadata

- Task ID: T-0132
- Legacy ID: ER-0044
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #180
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0044-Capability-Hooks-and-Policy-Plumbing.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #180
---

# ER-0044 — Capability Hooks and Policy Plumbing

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0044
- Title: Capability Hooks and Policy Plumbing
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v1 requires minimal capability hooks for kernel-grade enforcement without implementing full policy.
- Background / constraints: must integrate with Refract operation metadata and CEO runtime.

## Goals

- Add capability metadata hooks to operations and task execution paths.
- Provide plumbing for future policy enforcement.
- Surface capability requirements in operation listings.

## Non-Goals

- Full security policy engine or permission model.
- User authentication and identity management.

## Scope

- In scope: metadata hooks, enforcement stubs, and capability checks in call paths.
- In scope: minimal capability wiring for Conch operation calls.
- Out of scope: comprehensive security policy definitions.

## Requirements

- Functional: operations can declare required capabilities and checks are invoked.
- Non-functional: deterministic failure behavior for missing capabilities.

## Proposed Approach

- Summary: add capability fields in OperationDefinition and enforce checks in Conduit/CEO.
- Alternatives considered: defer capability plumbing to v2 (rejected).

## Acceptance Criteria

- Operations with required capabilities reject calls without required context.
- Capability metadata is visible in `show type` outputs.

## Risks / Open Questions

- Risk: capability checks may create backward compatibility issues.
- Question: what minimal capability set is required for v1?

## Dependencies

- Dependency 1: ER-0028 Conduit Operation Model.
- Dependency 2: ER-0043 CEO Runtime Hardening.

## Implementation Notes

- Notes for implementer: keep the capability model minimal and extensible.

## Verification Plan

- Tests to run: operation call tests with and without required capabilities.
- Manual checks: inspect metadata and attempt restricted calls.
```
