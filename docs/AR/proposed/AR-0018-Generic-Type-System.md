---
GitHub-Issue: #TBD
---

# AR-0018 — Generic Type System

- Status: Proposed
- Date: 2026-02-26
- Owners: Mike

## Context

IrisOS has introduced explicit type parameter metadata for Refract type definitions (e.g., Crate
collections and Astra math types). We need a full generic type system that makes those parameters
first-class: definable, instantiable, type-checkable, and encodable in Refract/Referee without
losing determinism or introspectability.

## Recommendation

Define a first-class Generic Type System in Refract that:

- Supports parameterized types with type parameters and value parameters.
- Supports variadic parameters (e.g., `Tensor<T, Dims...>`).
- Defines a canonical encoding for instantiated types and their parameters.
- Defines validation, substitution, and display rules that are deterministic across the system.

### Naming

Subsystem name: **Generics**.

## Goals

- Make type parameters and instantiations authoritative in Refract (not just metadata).
- Allow deterministic encoding of parameterized type IDs for storage and lookup.
- Allow Conch and system services to introspect and display generic instantiations.
- Enable future dispatch and overload rules to incorporate parameterized types.

## Non-Goals (v1)

- Advanced type inference across arbitrary expressions.
- Dependent types beyond simple value parameters (e.g., full symbolic constraints).
- Cross-language generic interop beyond Conch and internal services.

## Proposed Model (Broad Strokes)

### Parameter Kinds

- **Type parameter**: names a type (e.g., `T` in `List<T>`).
- **Value parameter**: names a value (e.g., `N` in `Vector<T, N>`).
- **Variadic parameter**: a list of parameters of a kind (e.g., `Dims...`).

### Instantiation

- A parameterized `TypeDefinition` may be instantiated with concrete arguments.
- Instantiation produces a canonical **GenericInstance** record in Refract.
- Instantiated types are addressable via a canonical TypeID encoding.

### Encoding (Sketch)

- A new `GenericInstance` schema encodes:
  - base type ID
  - ordered arguments (type/value/variadic)
  - canonicalized representation for deterministic TypeID derivation
- A stable TypeID derivation scheme (hash or tagged encoding) is required and must be
  deterministic across processes.

### Validation

- Argument arity must match parameters (including variadics).
- Argument kind must match parameter kind.
- Value parameters may define constraints (e.g., `N > 0`) in future revisions.

### Introspection

- `show type` should display parameter definitions and any instantiated arguments.
- `ls` should include instantiated types (if materialized) and base types.

## Open Questions

- TypeID encoding: explicit tagged encoding vs hashed ID?
- Where to store instantiated definitions: materialize in registry or compute on demand?
- How to represent value parameters (numeric vs structured)?
- Are constraints part of type parameters in v1?

## Next Steps

- Draft ERs to cover:
  - GenericInstance schema and TypeID encoding rules
  - Instantiation APIs and registry behavior
  - Conch syntax and display rules
  - Migration/compatibility for existing parameter metadata
