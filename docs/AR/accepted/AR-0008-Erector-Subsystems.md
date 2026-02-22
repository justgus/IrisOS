---
GitHub-Issue: #72
---

# AR-0008 — Erector Subsystems (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS needs a coherent set of foundational object types that represent logic, math, reflection,
execution primitives, and machine representation. These types must be portable, introspectable via
Refract, and suitable for system-level policies.

## Recommendation

Define **Erector** as the foundational object system for IrisOS, with the following subsystems:

- `Erector::Math` — mathematical and algebraic objects
- `Erector::Refract` — reflection objects (delegates to Refract graph)
- `Erector::Machine` — representation primitives and machine descriptors/handles
- `Erector::Exec` — synchronization primitives built on CEO
- `Erector::Comms` — communication objects and protocol stacks

### Erector::Math

- Numbers, Strings, Vectors, Matrices, Tensors
- Deterministic, portable behavior

### Erector::Refract

- A thin, typed facade over the Refract reflection graph
- Used by shells and tools to inspect types and operations

### Erector::Machine

Split into three layers to keep semantics clear:

1) **Representation primitives (pure, portable)**
   - Bit, Nibble, Byte, Word16/32/64, UInt128
   - Endianness, Alignment, Address, Pointer (as values)
   - Blob, Slice, Span, Packet
   - Checksum, Hash, UUID (ObjectID may live here or in Referee core)

2) **Resource descriptors (facts about the machine)**
   - Processor, Core, NUMANode
   - MemoryMap, MemoryRegion, Cache, TLB
   - RegisterFile, Register (architectural)
   - Device, Bus, Peripheral

3) **Capabilities + handles (permission to act)**
   - CoreLease, MemoryHandle, DMAHandle, DeviceHandle
   - IOPort, MMIORegionHandle
   - PacketPort, Channel

Definition vs Instance naming pattern:
- Definition = what it is
- Descriptor = what exists here
- Lease/Handle = permission to act

### Erector::Exec

- Mutex, Semaphore, Event, Future
- Waitable/Awaitable primitives implemented on CEO `await(Waitable&)`
- Provides coordination without blocking OS threads directly

### Erector::Comms

- Communication objects and protocol stacks (see AR-0009)

## Notes

Erector types are the building blocks for all higher-level system objects. They should remain
small, composable, and fully described in Refract.
