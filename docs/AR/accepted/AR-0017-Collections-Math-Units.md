---
GitHub-Issue: #81
ER-Dependencies: ER-0031, ER-0032, ER-0033, ER-0039, ER-0090, ER-0093, ER-0094, ER-0096, ER-0097, ER-0098
---

# AR-0017 — Collections, Math Types, and Units

- Status: Pending Review
- Date: 2026-02-21
- Owners: Mike

## Implementation Review — 2026-10-20

The System Engineer returned this AR to `Pending Review` after the
[implementation audit](../../Plans/AR-Implementation-Audit-2026-10-20.md) identified
implementation gaps (F8). This status supersedes earlier completion claims while the
remaining work and ER coverage are reviewed. The architectural decision remains accepted;
existing implementation evidence and ER verification records are preserved.

## Context

IrisOS needs robust, efficient collection types and richer mathematical primitives to support
analysis and data processing. Today, Refract bootstraps a minimal set (String, U64, Bool, Bytes).
We also need consistent handling for units of measure and conversions to avoid ambiguity and
errors in computation.

## Recommendation

Expand Refract's core type system with:

- **Collections**: Array, List, Set, Map, Tuple, Bytes (already present) and related operations.
- **Math primitives**: Float, Double, Vector, Matrix, Tensor.
- **Units of measure**: a unit system with conversions and dimension metadata.

Treat these as first-class Refract types with definitions stored in the Refract registry.

### Naming

- Collections subsystem: **Crate**
- Math subsystem: **Astra**
- Units subsystem: **Caliper**

## Goals

- Provide a common collection vocabulary usable by Conch and system services.
- Enable mathematical operations using well-defined numeric types and structures.
- Prevent unit confusion by attaching dimension metadata to values.
- Keep definitions introspectable and versioned in Refract.

## Non-Goals (v1)

- Full numeric tower with arbitrary precision.
- Automatic symbolic algebra.
- Full physical constants catalog.

## Proposed Model (Broad Strokes)

### Collections

- **Array**: fixed-length, contiguous, element type.
- **List**: ordered, growable, element type.
- **Set**: unordered, unique elements, element type.
- **Map**: key/value pairs, key type + value type.
- **Tuple**: fixed-length, heterogeneous element list.
- **Bytes**: raw byte buffer (already in core).

Each collection should expose standard operations (size, iterate, index, contains) and support
type parameterization in Refract (e.g., `List<String>`).

`Set` declares the standard `index` operation as well. This schema declaration does not impose an
iteration order on an unordered Set.

### Math Types

- **Float**: 32-bit IEEE.
- **Double**: 64-bit IEEE.
- **Vector**: 1-D numeric collection with element type and length fixed in its type arguments.
- **Matrix**: 2-D numeric collection with element type, rows, and columns fixed in its type arguments.
- **Tensor**: N-D numeric collection with element type and extents fixed in its type arguments.

### Quantities

Define quantity types as first-class wrappers to combine values with constraints or units:

- **Angle**
- **Duration**
- **Span**
- **Range**
- **Percentage**
- **Ratio**

### Units of Measure

- Define **Unit** and **Dimension** metadata objects in Refract.
- Values may carry optional unit metadata (e.g., meters, seconds).
- Provide conversion rules and compatibility checks.

## Open Questions

- Resolved for this batch by ER-0094: retain the existing generic type-parameter labels and represent arguments through existing Type/Value/Variadic forms.
- Units attach to quantity values, not generic type identity.
- Vector/Matrix/Tensor shapes are fixed in their type arguments.

## Next Steps

- Draft ERs for collections, math types, and units.
- Define Refract schemas for parameterized types.
- Identify which operations are required for v1.

## Implementation Progress

- Implemented through PR #346, merged as `304d7bfa72c239037796614f5c2d7cbb6877f573`; all listed ER dependencies are Verified in the final batch reconciliation. PR #347 completed the Caliper catalog work under AR-0019 and merged as `e1648b79efdc2fd3a7e55089d444968fee7407f9`.
- PR #346 CI run `38089811858` and PR #347 CI run `38166633486` passed their required checks. Local validation and individual ER evidence are recorded in `docs/Plans/AR-Batch-Progress.md`.
