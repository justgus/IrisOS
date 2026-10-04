---
Legacy-ID: ER-0074
GitHub-Issue: #282
Source-Path: docs/ER/ER-0074-Comms-Protocol-Objects-and-Hardware-Mapping.md
---

# T-0170 — Comms Protocol Objects and Hardware Mapping

## Task Metadata

- Task ID: T-0170
- Legacy ID: ER-0074
- Status: Active
- Source Status: Proposed
- Epic: EP-002
- Sprint Assigned: SP-007
- Sprint Start Date: 2026-10-04
- GitHub Issue: #282
- AR Dependencies: AR-0010, AR-0024
- Date Requested: 2026-05-28
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0074-Comms-Protocol-Objects-and-Hardware-Mapping.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Airframe Planning

- Estimate: 13 points (approved umbrella estimate)
- Dependencies: T-0169 (legacy ER dependency ER-0073).
- Planned Acceptance Criteria:
  - Protocol metadata declares required transport semantics and Machine capabilities.
  - Compatibility checks accept supported mappings and reject incompatible mappings deterministically.
  - Protocol metadata and mapping decisions are inspectable without performing I/O or negotiation.
- Planned Verification: `make check`; inspect supported, unsupported, missing, and contradictory protocol mappings.
- Acceptance Test: `TEST-0014` (`make check` passed 29 of 29 test programs on 2026-10-04; System Engineer acceptance pending).
- Scope split: protocol metadata compatibility and executable framing are covered by T-0186 and T-0187.
- Decomposition: the previously delivered protocol compatibility and execution slices are represented by verified Tasks T-0186 and T-0187. This umbrella is scheduled in SP-007 to complete and verify its recorded acceptance criteria.

## Preserved Legacy ER Content

```md
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
- Status: Proposed
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

- Tests to run:
  - `make check`
- Manual checks:
  - inspect protocol metadata and mapping decisions for representative transports.
```
