# Epic Active

Epics listed here are drafted, active, or complete-pending-close and are the current focus of planning or execution.

---

Currently: **1 Active Epic**

---

## EP-002: Machine and Communications Foundations

**Status:** Active
**Owner:** System Engineer
**Start Date:** 2026-08-21
**Reactivated:** 2026-10-04
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
1. **AC-0011:** Machine scalar, buffer, and packet primitives, plus processor architecture, register, address-space, and memory-block types, have stable Refract registrations retrievable by canonical identifiers.
2. **AC-0012:** In-memory descriptors represent configured or discovered processor, register-file, memory-region, bus, and device facts; creating or querying a descriptor does not itself grant authority or persist it.
3. **AC-0013:** A Machine handle names a descriptor and its granted capabilities; operations outside those capabilities are rejected.
4. **AC-0014:** A lease has explicit `Active`, `Released`, or `Revoked` state. Release and revocation produce deterministic state and access results without wall-clock expiry.
5. **AC-0015:** A Comms transport and session can reference Machine resources, and the existing loopback path continues to work with its prior behavior.
6. **AC-0016:** Given protocol and transport metadata, compatibility queries return a deterministic compatible or incompatible result, including for missing or contradictory metadata.
7. **AC-0017:** An executable example sends a registered Machine packet over a compatible Machine-backed loopback session, decodes the received frame, and verifies the packet equals the original.
8. **AC-0018:** The protocol example rejects malformed framing and incompatible transports deterministically and requires neither network access nor a hardware driver.
9. **AC-0019:** The repository validation suite passes with the Machine and Comms additions enabled.

### Planned Acceptance Tests

| Test | Acceptance Criterion | Scope | Status |
| ---- | -------------------- | ----- | ------ |
| TEST-0001 | AC-0011 | Machine value and Refract registration coverage | Ready |
| TEST-0002 | AC-0012 | Descriptor construction, queries, and non-authority | Ready |
| TEST-0003 | AC-0013 | Handle capability enforcement | Ready |
| TEST-0004 | AC-0014 | Deterministic lease lifecycle | Ready |
| TEST-0005 | AC-0015 | Machine-backed transport/session and loopback regression | Ready |
| TEST-0006 | AC-0016 | Protocol/transport compatibility decisions | Ready |
| TEST-0007 | AC-0017 | Registered packet loopback round trip | Ready |
| TEST-0008 | AC-0018 | Malformed framing and incompatible transport rejection | Ready |
| TEST-0009 | AC-0019 | Repository regression suite | Ready |

These tests are planned and linked in Airframe; they have not been run as part of this planning change.

### Poker Estimates

| Umbrella Task | Estimate |
| ------------- | -------- |
| T-0166 | 21 |
| T-0167 | 8 |
| T-0168 | 13 |
| T-0169 | 8-13 |
| T-0170 | 13 |

T-0166 through T-0170 are assigned to active Sprint SP-007 to complete and verify their recorded acceptance criteria. The verified child Tasks listed below capture previously delivered increments.

### Related Sprints

| Sprint | Goal | Status |
| ------ | ---- | ------ |
| SP-002 | Registered Machine values and packets | Closed |
| SP-003 | Queryable processor and memory inventory | Closed |
| SP-004 | Authorized Machine resource acquisition | Closed |
| SP-005 | Machine-backed Comms transport and sessions | Closed |
| SP-006 | Protocol compatibility and executable framing | Closed |
| SP-007 | EP-002 umbrella scope completion | Active |

### Related Tasks

| Task | Title | Sprint | Status | Test Plan |
| ---- | ----- | ------ | ------ | --------- |
| T-0166 | Machine Representation Primitives | SP-007 | Implemented - Verification Pending | TEST-0010 |
| T-0167 | Machine Descriptors and Resource Facts | SP-007 | Implemented - Verification Pending | TEST-0011 |
| T-0168 | Machine Handles and Leases | SP-007 | Implemented - Verification Pending | TEST-0012 |
| T-0169 | Comms Transport and Session Objects | SP-007 | Implemented - Verification Pending | TEST-0013 |
| T-0170 | Comms Protocol Objects and Hardware Mapping | SP-007 | Implemented - Verification Pending | TEST-0014 |
| T-0177 | Registered Machine Scalar Primitives | SP-002 | Verified |  |
| T-0178 | Registered Machine Buffers and Packets | SP-002 | Verified |  |
| T-0179 | Registered Processor Architecture Model | SP-003 | Verified |  |
| T-0180 | Registered Memory Topology Model | SP-003 | Verified |  |
| T-0181 | In-Memory Machine Descriptors and Queries | SP-003 | Verified |  |
| T-0182 | Capability-Bearing Machine Handles | SP-004 | Verified |  |
| T-0183 | Deterministic Machine Lease Lifecycle | SP-004 | Verified |  |
| T-0184 | Machine-Backed Comms Transport | SP-005 | Verified |  |
| T-0185 | Comms Session Lifecycle | SP-005 | Verified |  |
| T-0186 | Protocol Metadata and Compatibility | SP-006 | Verified |  |
| T-0187 | Executable Registered-Packet Protocol | SP-006 | Verified |  |

*Last Updated: 2026-10-04 (EP-002 reactivated; remaining Task and AC test plans recorded)*

*Last Updated: 2026-10-04*
