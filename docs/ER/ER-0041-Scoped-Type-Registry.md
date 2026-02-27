---
GitHub-Issue: #TBD
---

# ER-0041 — Scoped Type Registry

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0041
- Title: Scoped Type Registry
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: generic instantiations should be computed on demand and cached in scoped registries.
- Background / constraints: scopes include function/operation, app, database, sandbox, and global.

## Goals

- Implement scoped type registries with parent lookup and promotion rules.
- Provide deterministic cache behavior for generic instantiations.

## Non-Goals

- Distributed registry sync.
- Policy-driven access control beyond minimal hooks.

## Scope

- In scope: scoped registry data structures and lookup APIs.
- Out of scope: global distributed type registry.

## Requirements

- Functional: registry caches and returns GenericInstance entries by scope.
- Non-functional: deterministic lookup order and eviction rules.

## Proposed Approach

- Summary: define a scoped registry hierarchy and implement promotion rules.
- Alternatives considered: single global registry (rejected for scope control).

## Acceptance Criteria

- Scoped registries resolve generic instances deterministically.
- Registry promotion follows defined policy rules.

## Risks / Open Questions

- Risk: cache growth without eviction policy.
- Question: what minimal promotion rules are needed in v1?

## Dependencies

- Dependency 1: ER-0040 Generic Type System Core.

## Implementation Notes

- Notes for implementer: log registry promotions for debugging.

## Verification Plan

- Tests to run: registry scope tests and promotion behavior tests.
- Manual checks: instantiate generic types in nested scopes.
