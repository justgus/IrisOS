# Active Sprint

Sprints listed here are currently in Planning or Active status and are the current execution focus.

Currently: **1 active Sprint**

---

## SP-005: Machine-Backed Comms Transport and Sessions

**Status:** Active
**Epic:** EP-002
**Goal:** Deliver deterministic Machine-backed transport and session objects while preserving the existing loopback data path.
**Start Date:** 2026-08-24
**Capacity:** 10 points

### Sprint Planning

**Sprint Objective:**
Connect Comms transport and session metadata to Machine descriptors and SP-004 authority without introducing real hardware access, remote networking, protocol negotiation, or framing behavior.

**Approved Model Boundary:**
- A transport has stable identity, stream or datagram semantics, and one Machine resource reference.
- A Machine resource reference is either descriptive or backed by an active authorized handle lease.
- Handle-backed transport construction rejects missing, released, revoked, owner-mismatched, or context-mismatched leases deterministically.
- Existing `Channel` and `DatagramPort` loopback behavior remains unchanged and supplies the initial data path.
- A session references two endpoints and one transport.
- Session states are `Created`, `Open`, `Closing`, and `Closed`.
- Session transitions are explicit and invalid transitions fail without changing state.
- Protocol metadata, compatibility rules, and executable framing are deferred to SP-006.

**Execution Order:**
1. Define transport identity, semantics, and Machine resource references.
2. Validate authorized handle-backed construction against SP-004 leases.
3. Define endpoints, sessions, and deterministic lifecycle transitions.
4. Exercise descriptor-backed and authorized loopback transports and session transitions.
5. Run the complete test suite and confirm existing Comms behavior remains stable.

### Assigned Tasks

| Task | Title | Points | Status |
| ---- | ----- | -----: | ------ |
| T-0184 | Machine-Backed Comms Transport | 5 | Active |
| T-0185 | Comms Session Lifecycle | 5 | Active |

### Assigned Issues

None.

### Sprint Notes
- The System Engineer approved the 10-point split and model direction on 2026-08-24.
- T-0186 and T-0187 moved to SP-006 because protocol compatibility depends on the completed transport and session boundary.
- Real hardware access, drivers, cross-host networking, dynamic negotiation, and protocol framing are out of scope.

*Last Updated: 2026-08-24 (SP-005 planning approved and Sprint activated)*
