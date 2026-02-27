---
GitHub-Issue: #TBD
---

# ER-0044 — Capability Hooks and Policy Plumbing

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0044
- Title: Capability Hooks and Policy Plumbing
- Status: Proposed
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

## Non-Goals

- Full security policy engine or permission model.
- User authentication and identity management.

## Scope

- In scope: metadata hooks, enforcement stubs, and capability checks in call paths.
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
