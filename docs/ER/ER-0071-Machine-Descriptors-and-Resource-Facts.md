---
GitHub-Issue: #279
AR-Dependencies: AR-0008, AR-0010, AR-0024
ER-Dependencies: ER-0070
---

# ER-0071 — Machine Descriptors and Resource Facts

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0071
- Title: Machine Descriptors and Resource Facts
- Status: Verified
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Machine primitives alone do not describe processors, memory, buses, or devices.
- Background / constraints: AR-0024 separates descriptive resource facts from capability-bearing handles.

## Goals

- Add Machine descriptor objects for processor, memory, bus, and device resources.
- Represent resource facts without granting access.
- Preserve a path for later handles and leases.

## Non-Goals

- Capability checks.
- Device drivers.
- Comms transport/session objects.

## Scope

- In scope: descriptor schemas, construction helpers, persistence or registry integration where existing patterns support it, and tests.
- Out of scope: active resource acquisition and hardware probing.

## Requirements

- Functional: descriptors can record stable resource facts.
- Functional: descriptors do not imply permission to access the described resource.
- Non-functional: descriptor ordering and identifiers are deterministic.

## Proposed Approach

- Summary: layer descriptor objects on top of ER-0070 primitives, keeping them descriptive only.
- Alternatives considered: combining descriptors and handles was rejected because AR-0024 distinguishes facts from access-bearing leases.

## Acceptance Criteria

- Tests verify processor, memory, bus, and device descriptor creation.
- Tests verify descriptors can be queried or round-tripped through the chosen registry path.
- Tests verify descriptors do not expose access operations.

## Risks / Open Questions

- Risk: persistence expectations may vary by descriptor type.
- Question: which descriptor facts are required for the first implementation versus deferred?

## Dependencies

- Dependency 1: ER-0070 Machine Representation Primitives.

## Implementation Notes

- Notes for implementer: keep descriptor data factual and side-effect free.

## Verification Plan

### Reconciliation Evidence (2026-10-13)

- `src/machine/definitions.cc`, `topology.cc`, and `inventory.cc` implement descriptive machine facts. `test_machine_inventory` passes 6 checks for definitions, containment/overlap validation, reference/query behavior, and Refract registration.
- Fresh `TMPDIR=/private/tmp make check` passed 31/31 programs outside the sandbox.
- Status remains Complete. Independent Live Pass and System Engineer verification remain Pending;
  automated tests do not establish a human-observed pass.

- Tests to run:
  - `make check`
- Manual checks:
  - create representative descriptors and inspect their facts.

### Verification Reconciliation — 2026-10-15

- System Engineer ruling: accepted independent code/API review and executable tests for AR-0024;
  the earlier Pending human-observation gate is superseded. Independent Live Pass: Not Required.
  This records a change in applicability, not a human-observed pass.
- Status Verified based on the criterion-level
  [independent report](../Plans/AR-Batches/reviews/AR-0024-independent-review-2026-10-15.md),
  passing focused 5/5-program and full 31/31-program runs, and successful exact-head Linux CI
  [38751271580](https://github.com/justgus/IrisOS/actions/runs/38751271580) for repair commit
  `982379e414cd4a887520f8bff1f22a1d3770e708`. No open implementation findings remain.
