---
GitHub-Issue: #354
AR-Dependencies: AR-0022
ER-Dependencies: ER-0057, ER-0060, ER-0062
---

# ER-0100 — Owned Memory Allocation Service

## Roles

- Implementation Engineer: implements and validates the batch scope.
- System Engineer: rules on architecture and records required human observations.

## ER Metadata

- ER ID: ER-0100
- Title: Owned Memory Allocation Service
- Status: Proposed
- Date: 2026-10-14
- Owners: Mike
- Type: Enhancement

## Context

AR-0022 Stage E requires allocation beyond ER-0062's memory-region metadata. The proposed scope
needs the System Engineer's ruling on ownership and allocation policy before implementation.

## Goals

- Expose real, zero-initialized process-owned byte buffers through a local ServiceObject.
- Bound total allocated bytes, bytes per subject, and allocation count.
- Enforce request-sender ownership for inspection and release.

## Non-Goals

- Raw pointers, physical addresses, paging, shared memory, MMIO, DMA, and persisted allocations.
- Replacing the metadata-only MemoryService or changing sandbox enforcement.

## Scope

- In scope: allocation/inspection/release IPC endpoints, configurable constructor limits,
  stable runtime allocation IDs, and typed failures.
- Out of scope: hardware memory and arbitrary read/write endpoints.

## Requirements

- Each allocation owns a zero-initialized buffer until release or handler destruction.
- Only the authenticated local envelope subject can inspect or release its allocations.
- Zero size, negative/fractional/overflow values, malformed selectors/CBOR, quota exhaustion,
  allocation failures, and cross-subject access fail without partial state or quota changes.
- Calls follow the existing serialized local ServiceHostManager contract; sender identity follows
  the existing trusted local envelope contract and is not remote authentication.

## Proposed Approach

A separate AllocationService owns buffers through RAII and derives ownership from request.sender.
It validates endpoint name, optional endpoint type, and message type against the same declared
operation before dispatch. Existing service host persistence covers descriptors, not buffers.
This approach is Proposed pending the System Engineer's Stage E ruling.

## Acceptance Criteria

1. IPC allocate returns an allocation ID and size backed by owned zero-initialized memory.
2. IPC inspect/release permit the owner, reject other subjects, and preserve correlation/sandbox.
3. Limits reject zero, excessive size, per-subject/total-byte exhaustion, and count exhaustion;
   successful release restores limits and a second release returns NotFound.
4. Malformed CBOR, invalid numeric sizes/IDs, conflicting or omitted endpoint selectors, and unknown
   operations return typed errors without changing allocations.
5. Descriptor host recovery is stopped/unbound; explicit rebinding starts a fresh empty allocator.

## Risks / Open Questions

- Architectural ruling pending: bounded process-owned allocation and subject ownership.
- Stored descriptors do not persist constructor limits or allocation buffers; binding is trusted
  local configuration, matching existing host recovery behavior.

## Dependencies

- AR-0022 and Verified ER-0057/0060/0062.

## Implementation Notes

- Keep the existing MemoryService API and behavior intact.
- Require independent review for public API, ownership, lifetime, and input boundaries.

## Verification Plan

- `./bootstrap.sh` and `./configure` if new modules/test targets require regeneration.
- `make -j4`
- `TMPDIR=/private/tmp make -C tests check TESTS='test_core_services test_service_host test_service_ipc test_capability_context'`
- `TMPDIR=/private/tmp make check`
- Independent review must run the targeted suite and attempt quota/ownership/selector counterexamples.

### Independent Live Pass

- Required: No for the proposed bounded local C++ service contract.
- Reason: owned buffers, deterministic quotas, lifecycle, and request authorization can be faithfully
  inspected and exercised through the local IPC/host APIs; no physical target or user observation.
- Result: Not Required, subject to architectural scope approval.
