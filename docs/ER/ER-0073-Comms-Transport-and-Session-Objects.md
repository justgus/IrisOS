---
GitHub-Issue: #281
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
- Status: Verified
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

### Reconciliation Evidence (2026-10-13)

- `src/comms/transport_session.cc` implements Machine-linked transports, explicit session references, and strict lifecycle transitions. `test_comms_transport_session` passes 13 checks covering these surfaces and the protocol/mapping follow-on.
- Fresh `TMPDIR=/private/tmp make check` passed 31/31 programs outside the sandbox.
- Status remains Complete. Independent Live Pass and System Engineer verification remain Pending;
  automated tests do not establish a human-observed pass.

- Tests to run:
  - `make check`
- Manual checks:
  - create a Machine-backed transport and session and inspect stored metadata.

### Verification Reconciliation — 2026-10-15

- System Engineer ruling: accepted independent code/API review and executable tests for AR-0024;
  the earlier Pending human-observation gate is superseded. Independent Live Pass: Not Required.
  This records a change in applicability, not a human-observed pass.
- Status Verified based on the criterion-level
  [independent report](../Plans/AR-Batches/reviews/AR-0024-independent-review-2026-10-15.md),
  passing focused 5/5-program and full 31/31-program runs, and successful exact-head Linux CI
  [38751271580](https://github.com/justgus/IrisOS/actions/runs/38751271580) for repair commit
  `982379e414cd4a887520f8bff1f22a1d3770e708`. No open implementation findings remain.
