---
Legacy-ID: ER-0070
GitHub-Issue: #278
Source-Path: docs/ER/ER-0070-Machine-Representation-Primitives.md
---

# T-0166 — Machine Representation Primitives

## Task Metadata

- Task ID: T-0166
- Legacy ID: ER-0070
- Status: Backlog
- Source Status: Proposed
- Epic: EP-002
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #278
- AR Dependencies: AR-0008, AR-0010, AR-0024
- Date Requested: 2026-05-28
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0070-Machine-Representation-Primitives.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #278
AR-Dependencies: AR-0008, AR-0010, AR-0024
---

# ER-0070 — Machine Representation Primitives

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0070
- Title: Machine Representation Primitives
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0024 calls for an explicit `Erector::Machine` layer, but the repository does not yet have Machine representation primitives.
- Background / constraints: Machine should ground later descriptor, handle, and Comms work without replacing existing loopback primitives.

## Goals

- Introduce the first Machine subsystem structure.
- Define primitive representation types such as bytes, words, addresses, blobs, slices, and packets.
- Establish stable schema identities for Machine primitives where needed.

## Non-Goals

- Processor, memory, bus, or device descriptors.
- Capability-bearing handles or leases.
- Comms transport/session behavior.

## Scope

- In scope: Machine primitive types, local tests, and minimal build integration.
- Out of scope: hardware discovery and driver models.

## Requirements

- Functional: Machine primitives can represent deterministic byte and address-oriented data.
- Functional: primitive identities are stable enough for later descriptor ERs.
- Non-functional: existing Erector and Comms behavior remains unchanged.

## Proposed Approach

- Summary: add a small `Erector::Machine` primitive layer before descriptor and handle work.
- Alternatives considered: adding Machine fields directly to Comms was rejected because AR-0024 requires a separate Machine track.

## Acceptance Criteria

- Tests verify primitive construction and equality behavior.
- Tests verify packet or slice bounds behavior.
- Existing Comms tests continue to pass.

## Risks / Open Questions

- Risk: primitive naming may need adjustment once descriptor objects are implemented.
- Question: should Machine primitives live under a new `src/machine/` tree or existing Erector sources?

## Dependencies

- Dependency 1: AR-0024 Erector Machine and Comms Delivery Tracks.

## Implementation Notes

- Notes for implementer: keep this ER below descriptor and policy semantics.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect the Machine primitive API and confirm it does not introduce Comms policy behavior.
```
