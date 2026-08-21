---
Legacy-ID: ER-0072
GitHub-Issue: #280
Source-Path: docs/ER/ER-0072-Machine-Handles-and-Leases.md
---

# T-0168 — Machine Handles and Leases

## Task Metadata

- Task ID: T-0168
- Legacy ID: ER-0072
- Status: Backlog
- Source Status: Proposed
- Epic: EP-002
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #280
- AR Dependencies: AR-0008, AR-0010, AR-0024
- Date Requested: 2026-05-28
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0072-Machine-Handles-and-Leases.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #280
AR-Dependencies: AR-0008, AR-0010, AR-0024
ER-Dependencies: ER-0058, ER-0060, ER-0071
---

# ER-0072 — Machine Handles and Leases

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0072
- Title: Machine Handles and Leases
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Machine descriptors describe resources, but later Comms and service work need capability-bearing handles and leases.
- Background / constraints: Service-plane capability context and boundary enforcement are verified prerequisites.

## Goals

- Define Machine handles for memory, device, and IO-region resources.
- Add lease metadata for ownership, lifetime, and capability context.
- Keep handles separate from descriptive resource facts.

## Non-Goals

- Full hardware driver implementation.
- Network protocol modeling.
- Bypassing service-boundary capability checks.

## Scope

- In scope: handle and lease data models, capability attachment, validation, and tests.
- Out of scope: actual hardware access.

## Requirements

- Functional: handles reference Machine descriptors and required capabilities.
- Functional: leases record deterministic owner and lifetime metadata.
- Functional: invalid or missing capability context is rejected where enforcement applies.
- Non-functional: descriptors remain usable without handles.

## Proposed Approach

- Summary: add access-bearing Machine handle objects on top of descriptors and service-plane capability context.
- Alternatives considered: using raw descriptor IDs as access handles was rejected because it would collapse facts and authority.

## Acceptance Criteria

- Tests verify handle creation for known descriptor types.
- Tests verify lease metadata round-trips.
- Tests verify capability validation rejects unauthorized access where applicable.

## Risks / Open Questions

- Risk: lease lifetime semantics may need alignment with future task scheduling.
- Question: what minimum lease states are needed before Comms integration?

## Dependencies

- Dependency 1: ER-0058 Capability Context Objects and Persistence.
- Dependency 2: ER-0060 Service Boundary Capability Enforcement.
- Dependency 3: ER-0071 Machine Descriptors and Resource Facts.

## Implementation Notes

- Notes for implementer: do not add real device access in this ER.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - create descriptor-backed handles and verify capability-aware lease metadata.
```
