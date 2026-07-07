---
Legacy-ID: ER-0049
GitHub-Issue: #185
Source-Path: docs/ER/ER-0049-Object-Graph-Bundles-and-Packages.md
---

# T-0145 — Object Graph Bundles and Packages

## Task Metadata

- Task ID: T-0145
- Legacy ID: ER-0049
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #185
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0049-Object-Graph-Bundles-and-Packages.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #185
---

# ER-0049 — Object Graph Bundles and Packages

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0049
- Title: Object Graph Bundles and Packages
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v2 needs a standard bundle/package format to distribute object graphs and schemas.
- Background / constraints: bundles must be deterministic and verifiable.

## Goals

- Define a bundle format for object graphs, schemas, and metadata.
- Provide tooling for packing/unpacking bundles.

## Non-Goals

- Signed package distribution or trust frameworks.
- Networked package repositories.

## Scope

- In scope: bundle format spec, pack/unpack tooling, metadata schema.
- Out of scope: repository hosting and dependency resolution.

## Requirements

- Functional: bundles can be created and unpacked to a Referee store.
- Non-functional: deterministic bundle contents and versioned formats.

## Proposed Approach

- Summary: define a simple container format with manifest + object/schema payloads.
- Alternatives considered: reuse existing package formats (defer).

## Acceptance Criteria

- Bundle can be created from a store and restored to another store.
- Bundle metadata lists schema and object contents.

## Risks / Open Questions

- Risk: format changes could break compatibility.
- Question: what minimum metadata is required for bundle integrity?

## Dependencies

- Dependency 1: ER-0034 Referee Storage Layout Implementation (v1).

## Implementation Notes

- Notes for implementer: keep the manifest minimal and versioned.
- Bundle format v1 uses a JSON manifest with hex-encoded CBOR payloads for definitions, objects, and edges.

## Verification Plan

- Tests to run: pack/unpack tests for demo graphs.
- Manual checks: inspect bundle contents and verify restoration.
```
