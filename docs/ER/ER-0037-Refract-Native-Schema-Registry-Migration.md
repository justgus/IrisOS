---
GitHub-Issue: #TBD
---

# ER-0037 — Refract-Native Schema Registry Migration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0037
- Title: Refract-Native Schema Registry Migration
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v1 requires Refract schemas to be stored and managed natively in Referee, not mirrored from C++ definitions.
- Background / constraints: Dual representation must be retired without breaking existing boot flows.

## Goals

- Make Refract-native schema registry authoritative.
- Remove or gate C++ bootstrap-only schema definitions.

## Non-Goals

- Full schema authoring UI or tooling.
- Automated migrations of all existing schema data.

## Scope

- In scope: registry storage, lookup APIs, and migration from dual representation.
- Out of scope: schema editing workflows beyond existing Conch commands.

## Requirements

- Functional: schema registry reads from Referee as the source of truth.
- Non-functional: deterministic startup and lookup behavior.

## Proposed Approach

- Summary: introduce a migration path that loads existing schemas into Referee and flips registry reads to Referee-native storage.
- Alternatives considered: keep dual representation indefinitely (rejected).

## Acceptance Criteria

- Registry reports schema data from Referee without C++ duplication.
- Conch can list and show schemas after migration.

## Risks / Open Questions

- Risk: bootstrapping required types before registry availability.
- Question: what minimum bootstrap schema is still required for v1?

## Dependencies

- Dependency 1: ER-0034 Referee Storage Layout Implementation (v1).
- Dependency 2: ER-0021 Conch Schema and Object Authoring.

## Implementation Notes

- Notes for implementer: keep a fallback path for recovery of corrupt schema stores.

## Verification Plan

- Tests to run: registry lookup tests after migration.
- Manual checks: create schema in Conch and verify persistence.
