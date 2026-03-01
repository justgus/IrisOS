---
GitHub-Issue: #TBD
---

# ER-0034 — Referee Storage Layout Implementation (v1)

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0034
- Title: Referee Storage Layout Implementation (v1)
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v1 requires a durable storage layout aligned with AR-0003/AR-0004 beyond the SQLite prototype.
- Background / constraints: Must preserve deterministic ObjectID/DefinitionID indexing and be crash-safe.

## Goals

- Implement the chosen segment/index storage layout (or documented equivalent).
- Provide deterministic indexes for TypeID/DefinitionID and edge metadata.

## Non-Goals

- Full distributed storage or replication.
- Advanced compression or encryption policies.

## Scope

- In scope: on-disk layout, index structures, record format, basic tooling.
- Out of scope: tooling for remote sync or sharding.

## Requirements

- Functional: objects and edges persist across restart with deterministic lookup.
- Non-functional: crash-safe writes and recoverable index state.

## Proposed Approach

- Summary: implement the storage layout described in AR-0003/AR-0004 and replace SQLite paths.
- Alternatives considered: continuing SQLite for v1 (rejected for kernel-grade persistence).

## Acceptance Criteria

- Referee uses the segment/index layout for new stores.
- Object and edge lookup works via new indexes.

## Risks / Open Questions

- Risk: migration path from SQLite must preserve existing data.
- Question: which minimal index set is required for v1 demo workloads?

## Dependencies

- Dependency 1: AR-0003 Storage Layout Strategy.
- Dependency 2: AR-0004 Index and Graph Storage.

## Implementation Notes

- Notes for implementer: keep the layout simple and document file formats.

## Verification Plan

- Tests to run: persistence tests across restart and crash recovery.
- Manual checks: create objects/edges, restart, and verify index lookups.
