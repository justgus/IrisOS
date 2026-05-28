---
GitHub-Issue: N/A
AR-Dependencies: AR-0010, AR-0024
ER-Dependencies: ER-0070, ER-0071, ER-0072
---

# ER-0073 — Comms Transport and Session Objects

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0073
- Title: Comms Transport and Session Objects
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Existing Comms work is loopback-oriented and not yet grounded in Machine descriptors and handles.
- Background / constraints: AR-0024 places Comms transport and session objects above the Machine layer.

## Goals

- Define Comms transport objects backed by Machine facts or handles.
- Define session objects that reference transport identity and lifecycle state.
- Preserve existing loopback behavior while adding Machine-backed structure.

## Non-Goals

- Full protocol stack implementation.
- Hardware driver implementation.
- Cross-host networking.

## Scope

- In scope: transport and session schemas, lifecycle metadata, and tests.
- Out of scope: protocol object taxonomy and hardware mapping rules beyond transport references.

## Requirements

- Functional: transports can reference Machine descriptors or handles.
- Functional: sessions can record endpoint, transport, and state metadata.
- Functional: existing loopback primitives continue to work.
- Non-functional: session records are deterministic and testable without network access.

## Proposed Approach

- Summary: add Comms transport/session objects as metadata and lifecycle surfaces above Machine handles.
- Alternatives considered: extending loopback primitives directly was rejected because AR-0024 requires hardware-grounded Comms types.

## Acceptance Criteria

- Tests verify transport creation over representative Machine-backed references.
- Tests verify session creation and state metadata.
- Existing Comms primitive tests continue to pass.

## Risks / Open Questions

- Risk: transport/session boundaries may need refinement once protocol objects are defined.
- Question: which transport states are required in the first implementation?

## Dependencies

- Dependency 1: ER-0070 Machine Representation Primitives.
- Dependency 2: ER-0071 Machine Descriptors and Resource Facts.
- Dependency 3: ER-0072 Machine Handles and Leases.

## Implementation Notes

- Notes for implementer: keep this metadata-driven until protocol behavior is accepted.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create a Machine-backed transport and session and inspect stored metadata.
