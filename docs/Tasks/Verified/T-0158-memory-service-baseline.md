---
Legacy-ID: ER-0062
GitHub-Issue: #269
Source-Path: docs/ER/ER-0062-Memory-Service-Baseline.md
---

# T-0158 — Memory Service Baseline

## Task Metadata

- Task ID: T-0158
- Legacy ID: ER-0062
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #269
- AR Dependencies: -
- Date Requested: 2026-05-26
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0062-Memory-Service-Baseline.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #269
---

# ER-0062 — Memory Service Baseline

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0062
- Title: Memory Service Baseline
- Status: Verified
- Date: 2026-05-26
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0022 Stage E calls for expanded core services after service identity, persistence, capability context, and sandbox hooks are established. Network services are too broad for the first Stage E slice, but the system needs a smaller core service that can model memory resources.
- Background / constraints: ER-0057 through ER-0061 established the local service substrate, registry behavior, capability context, boundary checks, and sandbox identity hooks. This ER intentionally keeps memory service behavior metadata-oriented and does not expose raw memory access.

## Goals

- Introduce a first Stage E Memory Service.
- Model baseline memory regions as RAM, flash, or read-only regions.
- Provide local service endpoints for registering, listing, and looking up memory regions.
- Expose deterministic mutability metadata for region kinds.

## Non-Goals

- Network service implementation.
- Object-system or Referee-access service implementation.
- Inner processor, register, or stack service implementation.
- Allocation, paging, virtual memory, MMIO, DMA, or raw memory read/write behavior.
- Real hardware discovery.

## Scope

- In scope: in-process Memory Service object.
- In scope: memory region metadata with ID, name, kind, base, and size.
- In scope: local IPC endpoints for register, list, and lookup.
- In scope: duplicate and invalid region rejection.
- Out of scope: persistence of memory regions through Referee.
- Out of scope: hardware-backed or sandbox-enforced memory access.

## Requirements

- Functional: callers can register RAM, flash, and read-only memory region metadata.
- Functional: callers can list registered memory regions through the service endpoint.
- Functional: callers can look up a region by ID through the service endpoint.
- Functional: duplicate region IDs and duplicate region names are rejected.
- Functional: empty names and zero-sized regions are rejected.
- Non-functional: the service remains local, deterministic, and testable through existing Service Plane IPC.

## Proposed Approach

- Summary: add a `MemoryService` implementation under the existing service subsystem and represent region registration/list/lookup requests as CBOR-encoded JSON payloads.
- Alternatives considered: a broader Stage E catalog covering memory, object-system, and inner processor services was deferred because each service has distinct policy and safety questions. A network service was deferred as too much scope for this first Stage E implementation.

## Acceptance Criteria

- Tests verify memory region registration through IPC.
- Tests verify listing registered regions through IPC.
- Tests verify lookup by region ID through IPC.
- Tests verify duplicate and invalid region rejection.
- Tests verify RAM, flash, and read-only mutability classification.

## Risks / Open Questions

- Risk: callers may assume region metadata implies real allocation or hardware access; this ER does not provide either.
- Question: should memory regions become persisted Referee objects in a follow-on ER?
- Question: should flash writes require a distinct endpoint and capability grant in a follow-on ER?

## Dependencies

- Dependency 1: AR-0022 Staged Service Plane Delivery.
- Dependency 2: AR-0005 Service Plane Model.
- Dependency 3: ER-0061 Sandbox Identity and Service Isolation Hooks.

## Implementation Notes

- Notes for implementer: keep this slice metadata-only.
- Notes for implementer: do not add allocation or raw read/write semantics until the policy model is explicit.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - register RAM, flash, and read-only regions through the memory service, then list and look them up.
```
