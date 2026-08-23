# Active Sprint

Sprints listed here are currently in Planning or Active status and are the current execution focus.

---

## SP-004: Authorized Machine Resource Acquisition

**Status:** Active
**Epic:** EP-002
**Goal:** Deliver authorized acquisition and deterministic release or revocation of Machine resources.
**Start Date:** 2026-08-23
**End Date:** TBD
**Capacity:** 13 points

### Sprint Planning

**Sprint Objective:**
Add explicit authority and deterministic ownership lifecycle around the immutable Machine descriptors delivered by SP-003 without introducing hardware access, persistence, or clock policy.

**Approved Model Boundary:**
- Handles reference immutable Machine descriptors by stable resource ID and do not duplicate resource facts.
- Memory, device, and IO-region handles remain distinct resource categories.
- Handle construction requires a capability context that authorizes the resource and requested access mode.
- Handles carry authority metadata but expose no real hardware-access implementation.
- A lease associates one owner and capability context with one handle.
- Lease states are limited to `Active`, `Released`, and `Revoked`.
- Release and revocation are terminal and deterministically invalidate subsequent authorized use.
- Ownership mismatch and invalid lifecycle transitions fail without mutating the lease.
- Wall-clock expiration, renewal, background cleanup, concurrent arbitration, descriptor mutation, persistence, drivers, and hardware access remain out of scope.

**Execution Order:**
1. Define descriptor-backed handle categories and capability validation.
2. Define lease identity, ownership, states, and valid transitions.
3. Integrate lease validity with authorized handle use.
4. Validate authorized and unauthorized construction, lifecycle transitions, ownership mismatch, and invalid use with the complete test suite.

**Review Gates:**

| Gate | Required Before Proceeding |
| ---- | -------------------------- |
| Planning gate | The System Engineer approved the model boundary and 13-point capacity on 2026-08-23. |
| Handle gate | Handle identity, resource reference, access mode, and capability validation are explicit and grant no hardware operations. |
| Lease gate | Ownership, terminal release/revocation, and invalid-transition behavior are deterministic and clock-free. |
| Integration gate | Released or revoked leases cannot authorize subsequent handle use. |
| Completion gate | New authorization and lifecycle tests pass with the existing `make check` suite. |

### Assigned Tasks

| Task | Title | Points | Status |
| ---- | ----- | -----: | ------ |
| T-0182 | Capability-Bearing Machine Handles | 5 | Active |
| T-0183 | Deterministic Machine Lease Lifecycle | 8 | Active |

### Assigned Issues

None.

### Sprint Notes
- SP-003 descriptors remain immutable, in-memory facts; SP-004 adds authority through separate handles and leases.
- The System Engineer approved the model boundary and 13-point capacity on 2026-08-23.
- T-0183 is estimated at 8 points because it includes ownership semantics, terminal lifecycle transitions, capability integration, and negative-path testing.

*Last Updated: 2026-08-23 (SP-004 model boundary and 13-point capacity approved; Sprint activated)*
