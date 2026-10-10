---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0032, ER-0094
---

# ER-0097 — Astra Vector, Matrix, and Tensor Shape Contract

## ER Metadata

- ER ID: ER-0097
- Title: Astra Vector, Matrix, and Tensor Shape Contract
- Status: Complete
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 requires vectors, matrices, and tensors to carry element type and shape/extents. The accepted contract fixes extents in type arguments; Tensor pack encoding is governed by ER-0094.

## Requirements

- Extents are fixed type arguments: Vector is `T,N`; Matrix is `T,R,C`; Tensor is `T,Dims...`.
- Vector, Matrix, and Tensor preserve element type and shape through construction, inspection, and persistence.
- Vector and Matrix have exactly one and two extents respectively; Tensor has rank at least one. Every extent is a positive U64 Value argument. Checked extent multiplication rejects product overflow before generic type construction or persistence. Current Astra definitions do not define a runtime collection payload, so element-count comparison is outside this ER; values carry the shape through their generic type identity.
- No hidden default converts between fixed and dynamic shapes.

## Dependencies

- ER-0032 Astra Math and Structured Types.
- ER-0094 Astra Generic Parameter Pack Contract.

## Acceptance Criteria

- Tests cover extent 1 and `UINT64_MAX`, zero and negative/wrong-kind rejection, exact Vector/Matrix ranks, Tensor rank 1 and multiple extents, U64 product overflow, and persistence round trips of GenericInstance type identity. They assert invalid arguments are rejected before a write and malformed persisted instances fail on read.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Decision

Shapes are fixed in type arguments. Extents are positive U64 Value arguments; Tensor uses the
variadic contract from ER-0094. Values cannot override type-level extents.

### Live Pass (Human)

- Required: No
- Reason: Automated schema/value and persistence tests cover shape behavior.

## Completion Record

- Vector and Matrix now require exact ranks and positive U64 extent Values; Tensor applies the ER-0094 pack contract. Checked multiplication rejects product overflow during type derivation, registration, and scoped resolution. Matching persisted shape records are validated on read.
- Local validation: `make -j CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'`; focused registry suite passed 1/1; full `make check` passed 30/30; `git diff --check` passed.
- Independent read-only review passed with no findings; reviewer reran the focused registry suite (1/1).
