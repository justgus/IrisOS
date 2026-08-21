# Active Sprint

Sprints listed here are currently in Planning or Active status and are the current execution focus.

---

## SP-003: Queryable Processor and Memory Inventory

**Status:** Active
**Epic:** EP-002
**Goal:** Deliver a registered, deterministic, in-memory processor and memory inventory.
**Start Date:** 2026-08-21
**End Date:** TBD
**Capacity:** 18 points

### Sprint Planning

**Sprint Objective:**
Construct and query a deterministic, fully registered in-memory description of a Machine without reading hardware, storing live register values, granting resource access, or persisting descriptors.

**Approved Model Boundary:**
- `ArchitectureDefinition` has a stable definition ID, name, byte order, address width, and ordered register and core definitions.
- `RegisterDefinition` has a stable definition ID, name, bit width, and role; it describes layout and contains no current register value.
- `CoreDescriptor` has a stable resource ID, architecture and core-definition references, logical index, and enabled fact.
- `RegisterFileDescriptor` identifies the register definitions present for a core and contains no mutable register contents.
- `AddressSpace` has a stable resource ID, address width, and ordered memory regions.
- `MemoryRegion` has a start address, size, alignment, and kind. Regions may overlap to represent firmware and memory-mapped views.
- `AvailableMemoryBlock` is contained within one region, and available blocks do not overlap each other. Availability is a fact, not allocation authority.
- `BusDescriptor` and `DeviceDescriptor` have stable resource IDs, type/name metadata, and parent relationships, with no driver handles or operations.
- `MachineInventory` owns immutable descriptor values, rejects duplicate IDs and invalid references, returns results in stable resource-ID order, and supports lookup and category enumeration.

**Execution Order:**
1. Define architecture, core, and register definitions and their stable Refract registrations.
2. Define address spaces, memory regions, and available blocks with deterministic topology validation.
3. Define processor, register-file, memory, bus, and device descriptors.
4. Build immutable inventory construction, reference validation, lookup, and ordered enumeration.
5. Validate representative relationships, invalid topology, registration, and query behavior with the complete test suite.

**Review Gates:**

| Gate | Required Before Proceeding |
| ---- | -------------------------- |
| Definition gate | Architecture, core, and register definitions have stable identities and contain no live machine state. |
| Topology gate | Address widths, containment, overlap policy, alignment, and zero-size behavior are explicit and deterministic. |
| Descriptor gate | Resource descriptors reference definitions and parents without granting authority or exposing operations. |
| Query gate | Duplicate and dangling identifiers fail construction; lookup and enumeration ordering are deterministic. |
| Completion gate | New registration, topology, descriptor, and query tests pass with the existing `make check` suite. |

### Assigned Tasks

| Task | Title | Points | Status |
| ---- | ----- | -----: | ------ |
| T-0179 | Registered Processor Architecture Model | 8 | Active |
| T-0180 | Registered Memory Topology Model | 5 | Active |
| T-0181 | In-Memory Machine Descriptors and Queries | 5 | Active |

### Assigned Issues

None.

### Sprint Notes
- The System Engineer approved the model boundary and 18-point capacity on 2026-08-21.
- The three Tasks remain together because only the combined inventory is independently executable.
- Descriptor persistence, live register values, authority, hardware probing, drivers, and resource access are out of scope.
- `Makefile.am` remains the source of truth for build integration; generated Autotools files are not edited directly.

*Last Updated: 2026-08-21 (SP-003 planning approved and Sprint activated)*
