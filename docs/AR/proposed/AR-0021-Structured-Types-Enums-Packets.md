---
GitHub-Issue: #TBD
---

# AR-0021 — Structured Types, Enums, and Packets

- Status: Proposed
- Date: 2026-02-27
- Owners: Mike

## Context

Refract currently has tuples but lacks a first-class way to define named, structured
data sets as reusable types. IrisOS also needs enums that are independent of the
underlying value type and a packet model for precise ordered binary layouts. These
types must be definable, introspectable, and operable through the same Refract
definition and operation system used elsewhere.

## Recommendation

Define first-class Structured Types in Refract:

- **Struct** for unordered named fields.
- **Packet** for ordered named bit sequences that define a binary layout.
- **Enum** as a distinct type with named tags and explicit value mapping.

These types integrate with Refract Definitions, Generic Types (AR-0018), and Core
Operations (AR-0020), and support arrays, inheritance, and polymorphism where
explicitly declared.

### Naming

Subsystem name: **StructuredTypes**.

## Goals

- Make structured data types first-class and introspectable.
- Allow deterministic serialization and validation for structured instances.
- Support operations on structs, packets, and enums via OperationDefinition.
- Support arrays of structured types and packetized data.
- Enable inheritance and polymorphism where it is explicitly declared.

## Non-Goals (v1)

- Automatic binary layout optimization across architectures.
- Full schema migration tooling beyond Definition versioning.
- Automatic enum value inference or implicit tag assignment.

## Proposed Model (Broad Strokes)

### Structs

- A Struct is an unordered set of fields with `name`, `type`, and optional
  constraints.
- Field ordering is not semantically meaningful but can be persisted for display.
- Structs can define default values and required/optional fields.

### Packets

- A Packet is an ordered collection of named bit sequences.
- Each packet field declares bit width and a value type.
- Packets define a canonical binary layout and optional byte order.
- Packet fields may reference other packet definitions for nested layouts.

### Enums

- Enums define named tags mapped to values.
- Enum identity is independent of its underlying value type.
- Underlying type is explicit, but enum operations refer to the enum type.

### Arrays and Collections

- Arrays are first-class and can contain primitives, structs, packets, or enums.
- Arrays of structs and arrays of packets are defined as normal collection types.
- Multi-dimensional arrays are supported via generic value parameters.

### Inheritance and Polymorphism

- Structs and packets may extend other structs/packets by adding fields.
- Polymorphism is explicit and enforced by Definition metadata.
- Base and derived definitions must maintain deterministic serialization rules.

### Operations

- Operations apply to structured types and enums through OperationDefinition.
- Shared operations can be defined on base structured types and inherited.
- Enum operations may include comparison, formatting, and mapping helpers.

### Serialization and Validation

- Structs serialize by field name mapping with deterministic ordering rules.
- Packets serialize by their declared bit sequence order.
- Validation checks type constraints and packet bit widths at boundaries.

### Integration Points

- Generic value parameters may reference structs, packets, or enums (AR-0018).
- Core rendering and `print` defaults apply to structured types (AR-0020).
- Structured types are discoverable via `show type` in Conch.

## Open Questions

- Canonical binary layout rules for packets across architectures.
- How to represent and enforce polymorphic packet inheritance in binary layouts.
- Enum tagging conventions for interop and display.
