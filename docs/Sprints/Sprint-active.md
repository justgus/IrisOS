# Active Sprint

Sprints listed here are currently in Planning, Active, or Review status and are the current execution focus.

Currently: **1 review Sprint**

---

## SP-006: Protocol Compatibility and Executable Framing

**Status:** Review
**Epic:** EP-002
**Goal:** Deliver deterministic protocol compatibility and one bounded registered-packet round trip.
**Start Date:** 2026-08-24
**Capacity:** 13 points

### Sprint Planning

**Sprint Objective:**
Define inspectable compatibility between protocol metadata and SP-005 transports, then demonstrate
one bounded registered `Machine::Packet` round trip over an in-memory stream loopback.

**Approved Model Boundary:**
- Protocol metadata has stable identity, one required transport semantic, and a nonempty set of
  allowed Machine resource kinds.
- Compatibility produces an inspectable accepted or rejected result with a deterministic reason.
- Descriptor-backed transports are checked against their recorded Machine resource kind.
- Lease-backed transports resolve their Machine resource kind through the stored lease and must
  still pass lease, owner, and capability-context authorization at compatibility time.
- Compatibility performs no dynamic negotiation or transport IO.
- The executable example requires `TransportSemantics::Stream`, an `Open` session referencing the
  supplied transport, and an existing `Channel::loopback()` data path.
- A frame is a 4-byte unsigned big-endian payload length followed by exactly that many bytes.
- The maximum frame payload is 1 MiB (1,048,576 bytes).
- Oversized payloads or declarations, truncated payloads, trailing bytes, inactive leases,
  incompatible transports, and invalid session state fail deterministically.
- Datagram protocol execution, multiple frames per buffer, routing, dynamic negotiation, real
  networking, and hardware drivers remain out of scope.

**Execution Order:**
1. Define stable protocol metadata and deterministic compatibility result/reason types.
2. Resolve descriptor-backed and active lease-backed transport resource kinds.
3. Implement accepted and rejected compatibility mappings without negotiation or IO.
4. Implement bounded big-endian frame encoding and exact single-frame decoding.
5. Execute one registered-packet round trip over an open compatible stream loopback session.
6. Validate malformed frames, invalid sessions, inactive leases, incompatible mappings, and
   existing Comms behavior with the complete test suite.

**Review Gates:**

| Gate | Required Before Proceeding |
| ---- | -------------------------- |
| Planning gate | The System Engineer approved the model boundary and 13-point capacity on 2026-08-24. |
| Compatibility gate | Results and rejection reasons are inspectable, deterministic, and IO-free. |
| Framing gate | The 1 MiB single-frame bound and exact decode failures are explicit. |
| Execution gate | Only an open, compatible stream session can execute the loopback round trip. |
| Completion gate | New protocol tests pass with the existing `make check` suite. |

### Assigned Tasks

| Task | Title | Points | Status |
| ---- | ----- | -----: | ------ |
| T-0186 | Protocol Metadata and Compatibility | 5 | Implemented - Not Verified |
| T-0187 | Executable Registered-Packet Protocol | 8 | Implemented - Not Verified |

### Assigned Issues

None.

### Sprint Notes
- The System Engineer approved the compatibility model, 1 MiB frame bound, stream-loopback
  execution path, and 13-point capacity on 2026-08-24.
- T-0187 is 8 points because it integrates framing, session/transport compatibility, lease
  revalidation, bounded execution, and malformed-input tests.
- SP-005 transport and session types remain metadata-only; existing loopback primitives remain the
  data path.
- The System Engineer approved rejecting packet execution unless `Machine::Packet` is registered
  in Refract.
- Implementation validation passed all 29 test programs, including 12 Comms transport, session,
  compatibility, framing, and execution checks.
- The Sprint remains in Review until the System Engineer verifies both assigned Tasks.

*Last Updated: 2026-08-24 (SP-006 implementation complete; awaiting verification)*
