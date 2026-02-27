---
GitHub-Issue: #TBD
---

# ER-0048 — Referee Import/Export Tools

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0048
- Title: Referee Import/Export Tools
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v2 needs batch import/export of Referee objects and Refract schemas.
- Background / constraints: data must round-trip deterministically.

## Goals

- Provide export of object graphs and schemas to a portable format.
- Provide import workflows that preserve ObjectID and DefinitionID relationships.

## Non-Goals

- Full synchronization or replication of remote stores.
- Schema migration during import.

## Scope

- In scope: CLI tools or Conch commands for export/import.
- Out of scope: distributed sync or conflict resolution.

## Requirements

- Functional: export/import for object graphs and schemas.
- Non-functional: deterministic output and stable format.

## Proposed Approach

- Summary: implement import/export utilities using a defined bundle format.
- Alternatives considered: ad-hoc JSON exports (rejected for stability).

## Acceptance Criteria

- Exported data can be imported into a fresh store with correct references.
- Schema and object graphs remain consistent after import.

## Risks / Open Questions

- Risk: large exports may be slow without streaming.
- Question: what is the minimal export format for v2?

## Dependencies

- Dependency 1: ER-0049 Object Graph Bundles and Packages.

## Implementation Notes

- Notes for implementer: keep formats versioned and documented.

## Verification Plan

- Tests to run: export/import round-trip tests.
- Manual checks: export a demo object graph and re-import.
