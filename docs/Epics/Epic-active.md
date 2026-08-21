# Epic Active

Epics listed here are drafted, active, or complete-pending-close and are the current focus of planning or execution.

---

## EP-001: Migrate IrisOS Workflow Documentation to Agile Airframe

**Status:** Active
**Owner:** System Engineer
**Start Date:** 2026-07-06
**Target Close Date:** TBD
**Close Date:** TBD

**Goal:**
Move IrisOS from the manually maintained AR/ER/DR workflow into the Agile Airframe canonical workflow structure while preserving the current manual documentation state.

**Rationale:**
IrisOS currently tracks architecture recommendations, engineering requests, defect reports, and GitHub issue state through manually maintained documents. Agile Airframe provides a canonical workflow structure for Epics, Sprints, Tasks, Issues, GitHub mapping, and deterministic status projection. The migration must preserve existing IrisOS state without converting ARs into delivery Epics and without inventing historical Sprint records.

**Scope:**
- Add IrisOS Agile Airframe workspace configuration.
- Add canonical Airframe planning folders and indexes.
- Preserve ARs as separate architecture records.
- Migrate ERs into Airframe Tasks using Task numbers extended from the current ER list.
- Migrate DRs into Airframe Issues.
- Preserve GitHub issue references and document any local/GitHub state drift before mutation.
- Start Airframe Sprint tracking with SP-001 only; do not construct Sprint history.
- Provide local validation and review evidence for the migration.

**Out of Scope:**
- Reclassifying ARs as Epics.
- Reconstructing historical Sprints from existing implementation plans.
- Marking any ER, Task, DR, or Issue verified without System Engineer approval.
- Mutating GitHub issues, labels, titles, or bodies before a reviewed migration map is approved.
- Changing product behavior or C++ source code.

**Acceptance Criteria:**
1. IrisOS has a repo-local Airframe workspace configuration for `justgus/IrisOS`.
2. Airframe Epics, Sprints, Tasks, and Issues indexes exist and identify current active work.
3. ARs remain separate architecture records and are not converted into Epics.
4. SP-001 exists as the first Airframe Sprint and no historical Sprint archive is invented.
5. ER-derived Tasks preserve original ER IDs, titles, statuses, GitHub issue references, dependencies, and verification details.
6. DR-derived Issues preserve original DR IDs, titles, statuses, GitHub issue references, severity, reproduction, fix, and verification details.
7. A migration audit identifies all local/GitHub status mismatches before any GitHub mutation.
8. Generated or migrated indexes preserve the same canonical state as the current manual documentation.
9. GitHub issue migration steps are documented and require explicit approval before execution.
10. Validation commands and results are recorded before the migration is proposed for closeout.

### Planning Decisions

| Decision | Direction |
| -------- | --------- |
| Architecture records | Keep ARs as separate architecture records. Do not convert accepted ARs into Epics. |
| Sprint history | Start Airframe Sprint tracking with SP-001. Do not reconstruct historical Sprints. |
| Source of truth during migration | Treat current local AR/ER/DR documentation as the source of truth until a reviewed migration map says otherwise. |
| GitHub mutation | Prepare a mutation plan, but do not change GitHub titles, labels, bodies, or state before System Engineer approval. |
| Verification authority | Preserve human-only verification authority. Implementation work may become Implemented - Not Verified, but not Verified, without System Engineer action. |

### Closeout Gate

EP-001 may move to Complete only after:

1. All source AR, ER, and DR documents are represented in the approved migration map.
2. The canonical Airframe projections preserve the approved source status of every migrated item.
3. Known local/GitHub drift is either resolved or explicitly deferred with a follow-up Task.
4. The GitHub mutation plan is reviewed and either executed with approval or deferred with documented rationale.
5. Validation evidence is recorded for the final migrated documentation state.

### Related Sprints

| Sprint | Goal | Status |
| ------ | ---- | ------ |
| SP-001 | Plan and implement the Agile Airframe documentation migration seed, audit, canonical conversion, GitHub mapping plan, and cutover validation. | Closed |

### Related Tasks

| Task | Title | Status |
| ---- | ----- | ------ |
| T-0081 | Audit current AR/ER/DR documentation and GitHub issue state | Verified |
| T-0082 | Install IrisOS Airframe workspace configuration and canonical planning structure | Verified |
| T-0083 | Define separate architecture-record handling for ARs | Verified |
| T-0084 | Migrate ER records into Airframe Tasks | Verified |
| T-0085 | Migrate DR records into Airframe Issues | Verified |
| T-0086 | Generate Airframe indexes and GitHub issue mapping | Verified |
| T-0087 | Prepare approved GitHub issue synchronization migration | Verified |
| T-0088 | Validate migration equivalence and document cutover | Verified |

### Related Issues

| Issue | Title | Status |
| ----- | ----- | ------ |

### Notes

Task IDs begin at T-0081 to extend from the current highest IrisOS ER ID, ER-0080. GitHub issue fields remain TBD until the GitHub mutation plan is reviewed and approved.

*Last Updated: 2026-08-21 (SP-001 closed)*

---

## EP-002: Machine and Communications Foundations

**Status:** Active
**Owner:** System Engineer
**Start Date:** 2026-08-21
**Target Close Date:** TBD
**Close Date:** TBD

**Goal:**
Establish registered Machine representations, in-memory resource descriptors, capability-bearing handles and leases, and Machine-backed Comms protocol execution as required by AR-0024.

**Rationale:**
IrisOS has working loopback Comms primitives but no Machine layer on which hardware-grounded communication can be modeled. EP-002 supplies that layer in independently executable increments while keeping definitions, discovered instances, and authority distinct.

**Approved Architecture Boundary:**
- Registered Machine primitives define portable scalar values, buffers, packets, processor architectures, registers, address spaces, and memory-block types.
- Runtime descriptors represent discovered or configured processor cores, register files, installed memory regions, buses, and devices.
- Descriptors are in-memory and non-authoritative in this Epic; persistence is deferred.
- Handles and leases carry authority separately from resource facts.
- Initial leases use explicit `Active`, `Released`, and `Revoked` lifecycle states without wall-clock expiration.
- Comms transports and sessions are layered above Machine resources and culminate in one executable protocol round trip.

**Scope:**
- Fully register Machine primitives with Refract.
- Model processor architecture and memory topology.
- Provide deterministic, in-memory Machine descriptors and queries.
- Provide capability-aware handles and deterministic lease lifecycle behavior.
- Add Machine-backed transport and session metadata while preserving existing loopback behavior.
- Add inspectable protocol compatibility and one executable protocol example.

**Out of Scope:**
- Machine descriptor persistence.
- Wall-clock lease expiration.
- Hardware probing or device drivers.
- A complete network stack or remote networking.
- Dynamic protocol negotiation.

**Acceptance Criteria:**
1. Machine scalar, buffer, packet, processor-architecture, register, address-space, and memory-block types have stable Refract registrations.
2. Runtime descriptors model concrete Machine resources without granting access or requiring persistence.
3. Capability-bearing handles reference descriptors and reject unauthorized use.
4. Leases support deterministic active, released, and revoked behavior without depending on a clock.
5. Transports and sessions reference Machine resources without regressing existing loopback Comms behavior.
6. Protocol metadata can determine transport compatibility deterministically.
7. A registered Machine packet completes an executable encode, transport, decode, and verification round trip.
8. Existing tests continue to pass.

### Poker Estimates

| Umbrella Task | Estimate |
| ------------- | -------- |
| T-0166 | 21 |
| T-0167 | 8 |
| T-0168 | 13 |
| T-0169 | 8-13 |
| T-0170 | 13 |

Tasks above eight points are represented by Sprint-ready child Tasks. T-0166 through T-0170 remain backlog umbrella records to preserve legacy ER traceability.

### Related Sprints

| Sprint | Goal | Status |
| ------ | ---- | ------ |
| SP-002 | Registered Machine values and packets | Review |
| SP-003 | Queryable processor and memory inventory | Backlog |
| SP-004 | Authorized Machine resource acquisition | Backlog |
| SP-005 | Machine-backed communication with an executable protocol | Backlog |

### Related Tasks

| Task | Title | Sprint | Status |
| ---- | ----- | ------ | ------ |
| T-0166 - T-0170 | Legacy ER umbrella Tasks | N/A | Backlog |
| T-0177 | Registered Machine Scalar Primitives | SP-002 | Implemented - Not Verified |
| T-0178 | Registered Machine Buffers and Packets | SP-002 | Implemented - Not Verified |
| T-0179 | Registered Processor Architecture Model | SP-003 | Backlog |
| T-0180 | Registered Memory Topology Model | SP-003 | Backlog |
| T-0181 | In-Memory Machine Descriptors and Queries | SP-003 | Backlog |
| T-0182 | Capability-Bearing Machine Handles | SP-004 | Backlog |
| T-0183 | Deterministic Machine Lease Lifecycle | SP-004 | Backlog |
| T-0184 | Machine-Backed Comms Transport | SP-005 | Backlog |
| T-0185 | Comms Session Lifecycle | SP-005 | Backlog |
| T-0186 | Protocol Metadata and Compatibility | SP-005 | Backlog |
| T-0187 | Executable Registered-Packet Protocol | SP-005 | Backlog |

*Last Updated: 2026-08-21 (EP-002 planning approved and activated)*
