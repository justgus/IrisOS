# Active Sprint

Sprints listed here are currently in Planning or Active status and are the current execution focus.

---

## SP-002: Registered Machine Values and Packets

**Status:** Review
**Epic:** EP-002
**Goal:** Deliver fully Refract-registered Machine scalar, buffer, and packet primitives that can construct, inspect, and validate packet data.
**Start Date:** 2026-08-21
**End Date:** TBD
**Capacity:** 10 points

### Sprint Planning

**Sprint Objective:**
Establish the smallest independently executable Machine layer: registered portable values and bounded packet composition, without descriptors, authority, or Comms policy.

**Execution Order:**
1. Define scalar representation semantics and stable Refract identities.
2. Register scalar definitions and verify registry inspection.
3. Build blobs, slices, and packets over the scalar layer.
4. Register compound definitions and verify bounds and packet behavior.

**Review Gates:**

| Gate | Required Before Proceeding |
| ---- | -------------------------- |
| Scalar API gate | Byte order, widths, address value semantics, and naming are reviewable and contain no host-layout assumptions. |
| Registration gate | Stable identities and relationships are inspectable through Refract. |
| Packet gate | Bounds failures are deterministic and packet construction does not introduce transport behavior. |
| Completion gate | New tests and the existing `make check` suite pass. |

### Assigned Tasks

| Task | Title | Points | Status |
| ---- | ----- | -----: | ------ |
| T-0177 | Registered Machine Scalar Primitives | 5 | Implemented - Not Verified |
| T-0178 | Registered Machine Buffers and Packets | 5 | Implemented - Not Verified |

### Assigned Issues

None.

### Sprint Notes
- T-0166 remains the legacy ER umbrella Task for traceability.
- Processor architecture, register, and memory topology work begins in SP-003.
- This Sprint changes `Makefile.am` sources of truth if build integration is needed; generated Autotools files remain untouched.
- Implementation validation passed all 26 tests, including 8 Machine primitive checks.
- The Sprint remains in Review until the System Engineer verifies both Tasks.

*Last Updated: 2026-08-21 (SP-002 implementation complete; awaiting verification)*
