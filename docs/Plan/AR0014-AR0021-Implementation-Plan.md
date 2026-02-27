# Implementation Plan: AR-0014 through AR-0021 (v1)

- Status: Draft
- Date: 2026-02-27
- Owners: Mike

## Scope

This plan defines the v1 build, focused on persistence correctness and a viable kernel-grade
foundation. It treats the v0 plan as complete and builds on the accepted/proposed ARs up through
AR-0021. ER-0021 through ER-0033 are mapped into v1 or v2 as noted below.

## Guiding Principles

- Make persistence correct and durable before expanding userland scope.
- Promote Refract-native schemas and structured types to first-class status.
- Ensure the kernel and core libraries use the same operation/dispatch model.
- Keep the kernel core small and deterministic; defer utilities to v2.

---

## Phase 1 — Persistence and Storage Hardening

### Features

- Align Referee storage with AR-0003/AR-0004 beyond SQLite.
- Define durable on-disk layout, indexes, and recovery strategy.
- Formalize Definition versioning + migration hooks.

### Milestones

- M1.1: Storage layout selected and implemented (segment/index or equivalent).
- M1.2: Recovery and rebuild workflows validated.

### ERs

- ER-0018 Phase6 Storage Alignment (Verified)
- ER-0019 Phase6 Persistence Tests (Verified)
- ER-0020 Phase6 Integration (Verified)

### Tests

- Persistence across restart and crash recovery.
- Definition migration tests for versioned schemas.

---

## Phase 2 — Refract-Native Schemas and Structured Types

### Features

- Move schemas from dual representation to Refract-native storage as the source of truth.
- Implement Structured Types per AR-0021 (Struct/Packet/Enum) with validation + serialization.
- Make arrays of structured types first-class in Refract.

### Milestones

- M2.1: Refract-native schema registry is authoritative.
- M2.2: Struct/Packet/Enum definitions can be created, stored, and introspected.

### ERs

- ER-0021 Conch Schema and Object Authoring (Verified)
- ER-0031 Crate Collections (Verified)
- ER-0032 Astra Math Types (Verified)
- ER-0033 Caliper Units and Quantities (Verified)

### Tests

- Definition creation and lookup for structured types.
- Serialization round-trip tests for structs and packets.

---

## Phase 3 — Generics and Type Identity

### Features

- Implement AR-0018 generic type system with explicit TypeID encoding.
- Scoped type registries for caching generic instantiations.
- Deterministic instantiation validation and introspection.

### Milestones

- M3.1: GenericInstance schema and TypeID encoding implemented.
- M3.2: Scoped registry behaviors validated.

### Tests

- Generic instantiation determinism tests.
- TypeID stability tests across processes.

---

## Phase 4 — Core Operations and Dispatch Integration

### Features

- Implement AR-0020 core operations (`to_string`, `print`, `render`, compare semantics).
- Implement Conduit operation registry and dispatch per AR-0016/ER-0028..0030.
- Bind operations to implementations across core types and structured types.

### Milestones

- M4.1: Core operation metadata and bindings in place for primitives.
- M4.2: Conch validates and dispatches operations via Conduit.

### ERs

- ER-0028 Conduit Operation Model (Verified)
- ER-0029 Conduit Dispatch Engine (Verified)
- ER-0030 Conduit Conch Integration (Verified)

### Tests

- Dispatch resolution tests for overloads and inheritance.
- Conch `call` validation and invocation tests.

---

## Phase 5 — Kernel Runtime Hardening

### Features

- Strengthen CEO task runtime for long-lived kernel execution.
- Formalize resource and capability hooks (minimal policy, strict plumbing).
- Define I/O primitives that map to kernel execution contexts.

### Milestones

- M5.1: Task runtime stability under sustained load.
- M5.2: Capability hooks present for policy enforcement in v2.

### Tests

- Long-run task lifecycle tests.
- I/O wait/wake stress tests.

---

## Phase 6 — v1 Integration and Kernel Demo

### Features

- End-to-end kernel-grade demo: Conch + Referee + Refract + CEO + Conduit + Structured Types.
- Document v1 limitations and v2 roadmap.

### Milestones

- M6.1: Kernel demo succeeds with persistent structured data.
- M6.2: v2 roadmap published and agreed.

---

## ER Placement Summary

### v1 (core kernel and persistence)

- ER-0021, ER-0022, ER-0023, ER-0028, ER-0029, ER-0030, ER-0031, ER-0032, ER-0033

### v2 (utilities and tooling)

- ER-0024, ER-0025, ER-0026, ER-0027

---

## Open Questions

- Exact storage backend choice and migration path from SQLite.
- Minimum capability/security model required for kernel-grade operation.
