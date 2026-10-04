# Active Sprint

Sprints listed here are currently in Planning or Active status and are the current execution focus.

---

Currently: **1 Active Sprint**

## SP-007: EP-002 Umbrella Scope Completion

**Status:** Active
**Epic:** EP-002 - Machine and Communications Foundations
**Start Date:** 2026-10-04
**Capacity:** Not set
**Estimated Scope:** 63-68 points

**Goal:** Complete and verify the five EP-002 umbrella Tasks for Machine representations, resource descriptors, capability handles and leases, Comms transports and sessions, and protocol mapping.

The estimate range totals 63-68 points: T-0166 (21), T-0167 (8), T-0168 (13), T-0169 (8-13), and T-0170 (13). Work is planned in dependency order. T-0168 also depends on verified Tasks T-0154 and T-0156. Verified child Tasks record previously delivered increments; the umbrella Tasks still require completion against their own acceptance criteria.

| Order | Task | Scope | Estimate | Dependencies | Acceptance Test |
| ----- | ---- | ----- | -------- | ------------ | --------------- |
| 1 | [T-0166](../Tasks/Active/T-0166-machine-representation-primitives.md) | Machine representation primitives | 21 | AR-0008, AR-0010, AR-0024 | TEST-0010 |
| 2 | [T-0167](../Tasks/Active/T-0167-machine-descriptors-and-resource-facts.md) | Machine descriptors and resource facts | 8 | T-0166 | TEST-0011 |
| 3 | [T-0168](../Tasks/Active/T-0168-machine-handles-and-leases.md) | Machine handles and leases | 13 | T-0154, T-0156, T-0167 | TEST-0012 |
| 4 | [T-0169](../Tasks/Active/T-0169-comms-transport-and-session-objects.md) | Comms transport and session objects | 8-13 | T-0166, T-0167, T-0168 | TEST-0013 |
| 5 | [T-0170](../Tasks/Active/T-0170-comms-protocol-objects-and-hardware-mapping.md) | Comms protocol objects and hardware mapping | 13 | T-0169 | TEST-0014 |

The implementation tests passed 29 of 29 programs on 2026-10-04 before the packaging fix. After the packaging fix, the focused Comms suite passed all 13 checks. A current full `make check` run passes 28 of 29 programs; the pre-existing `test_referee_core` binary cannot load `/usr/local/lib/libreferee.0.dylib` on this macOS host. Local `make distcheck` stops earlier on pre-existing `-Werror` diagnostics in `src/ceo/io_reactor.cc`, before reaching installation. The five Tasks are Implemented and pending System Engineer verification.

*Last Updated: 2026-10-04*
