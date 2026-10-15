---
GitHub-Issue: #282
AR-Dependencies: AR-0010, AR-0024
ER-Dependencies: ER-0073
---

# ER-0074 — Comms Protocol Objects and Hardware Mapping

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0074
- Title: Comms Protocol Objects and Hardware Mapping
- Status: Verified
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Transport/session objects need protocol metadata and explicit hardware mapping before Comms can move beyond local primitives.
- Background / constraints: AR-0024 keeps protocol objects above Machine descriptors and handles.

## Goals

- Define protocol metadata objects for Comms.
- Map protocols to compatible transports and Machine-backed resources.
- Keep protocol metadata inspectable and deterministic.

## Non-Goals

- Complete network stack.
- Real hardware drivers.
- Dynamic protocol negotiation.

## Scope

- In scope: protocol object schema, compatibility metadata, hardware mapping rules, and tests.
- Out of scope: packet routing engines and remote network behavior.

## Requirements

- Functional: protocols can declare required transport and Machine capabilities.
- Functional: compatibility checks can determine whether a protocol maps to a transport.
- Functional: incompatible mappings fail deterministically.
- Non-functional: existing session behavior remains stable.

## Proposed Approach

- Summary: add explicit protocol objects and compatibility checks over ER-0073 transports and sessions.
- Alternatives considered: implicit protocol naming conventions were rejected because mappings must be inspectable.

## Acceptance Criteria

- Tests verify compatible protocol-to-transport mappings.
- Tests verify incompatible mappings fail deterministically.
- Tests verify protocol metadata is inspectable.

## Risks / Open Questions

- Risk: the first protocol taxonomy may be too narrow.
- Question: which protocol examples should anchor the first implementation?

## Dependencies

- Dependency 1: ER-0073 Comms Transport and Session Objects.

## Implementation Notes

- Notes for implementer: model compatibility metadata before protocol execution behavior.

## Verification Plan

### Reconciliation Evidence (2026-10-13)

- `src/comms/transport_session.cc` implements inspectable protocol metadata, deterministic compatibility checks, Machine capability mappings, and a bounded loopback packet path. `test_comms_transport_session` passes 13 checks including incompatible mappings and lease revalidation.
- Fresh `TMPDIR=/private/tmp make check` passed 31/31 programs outside the sandbox.
- Status remains Complete. Independent Live Pass and System Engineer verification remain Pending;
  automated tests do not establish a human-observed pass.

- Tests to run:
  - `make check`
- Manual checks:
  - inspect protocol metadata and mapping decisions for representative transports.

### Verification Reconciliation — 2026-10-15

- System Engineer ruling: accepted independent code/API review and executable tests for AR-0024;
  the earlier Pending human-observation gate is superseded. Independent Live Pass: Not Required.
  This records a change in applicability, not a human-observed pass.
- Status Verified based on the criterion-level
  [independent report](../Plans/AR-Batches/reviews/AR-0024-independent-review-2026-10-15.md),
  passing focused 5/5-program and full 31/31-program runs, and successful exact-head Linux CI
  [38751271580](https://github.com/justgus/IrisOS/actions/runs/38751271580) for repair commit
  `982379e414cd4a887520f8bff1f22a1d3770e708`. No open implementation findings remain.
