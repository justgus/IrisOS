---
GitHub-Issue: #TBD
---

# AR-0020 — Core Type Operations and Rendering

- Status: Proposed
- Date: 2026-02-27
- Owners: Mike

## Context

IrisOS has introduced a growing set of Refract types (core primitives, Crate collections,
Astra math types, Caliper units/quantities). To make these types usable from Conch and
system services, we need a standardized, extensible operations surface and a rendering
model that supports both text and graphical contexts.

## Recommendation

Define a system-wide, phased operations model that:

- Establishes standard operations for core primitives (String, U64, Bool, Bytes, ObjectID).
- Separates `to_string`, `print`, and `render` semantics across all types.
- Binds Refract operation definitions to concrete implementations.
- Enables Conch to invoke operations on objects and print results.
- Defers generic-dependent operations to a later phase aligned with AR-0018.

### Naming

Subsystem name: **CoreOps**.

## Goals

- Make core type operations discoverable, introspectable, and callable.
- Provide deterministic text output (`to_string` and `print`) across contexts.
- Define `render` as a context-aware display hook for future UI systems.
- Allow types to opt out of display operations when appropriate.

## Non-Goals (v1)

- Full language-level operator overloading.
- Rich GUI rendering system or UI framework (render will be a hook only).
- Exhaustive operation sets for all generic collections until AR-0018 is implemented.

## Proposed Model (Broad Strokes)

### Standard Operations (Optional but Standardized)

All types may implement the following operations, but they are **not required**:

- `to_string() -> String`: pure string representation with no side effects.
- `print() -> Void`: writes text to the current console/context (typically calls `to_string`).
- `render(context) -> Void`: context-aware display hook; Conch uses `print` as default.

### Core Primitive Operation Sets (Initial)

**String**
- `equals(other:String) -> Bool`
- `compare(other:String) -> Int` (signed; exact semantics in ER)
- `concat(other:String) -> String`
- `contains(substr:String) -> Bool`
- `starts_with(prefix:String) -> Bool`
- `ends_with(suffix:String) -> Bool`
- `split(delim:String) -> List<String>` (requires Crate + generics)
- `slice(start:U64, len:U64?) -> String`
- `replace(target:String, repl:String) -> String`
- `to_upper() -> String`
- `to_lower() -> String`
- `length() -> U64`

**U64**
- `add/sub/mul/div/mod` (exact names to be defined in ER)
- `compare(other:U64) -> Int` (signed; exact semantics in ER)
- `to_string() -> String`

**Bool**
- `not() -> Bool`
- `and/or/xor(other:Bool) -> Bool`
- `to_string() -> String`

**Bytes**
- `length() -> U64`
- `slice(start:U64, len:U64?) -> Bytes`
- `to_hex() -> String`
- `to_string() -> String` (explicit encoding rules in ER)

**ObjectID**
- `to_string() -> String`
- `equals(other:ObjectID) -> Bool`

### Rendering Semantics

- `to_string` is pure and does not assume any display context.
- `print` writes to the active console/context and may call `to_string`.
- `render` is context-aware and may produce UI/graphics; in Conch it defaults to `print`.
- `print` exists independent of Conch to support other tools such as compilers and debuggers.
- When `print` is not defined for an instance, default output is its ObjectID.
- When `print` is not defined for a type, default output is the type's standard name.

### Dispatch & Binding

- Refract stores operation definitions as metadata.
- A binding layer maps operation definitions to concrete implementations.
- Conch uses the binding layer to call operations by name on objects.

## Phases

1. **Core Operation Definitions**
   - Add operation metadata for core primitives.
   - Define `to_string`, `print`, `render` standard semantics.

2. **Execution and Conch Integration**
   - Bind operations to implementations.
   - Add `print <ObjectID>` and `render <ObjectID>` commands in Conch.

3. **Rendering Contexts**
   - Define render context interfaces and default behaviors.
   - Conch render defaults to print.

4. **Collections and Math Types**
   - Expand operation sets for Crate/Astra types.
   - Implement rendering for Vector/Matrix/Tensor with shape-aware output.

5. **Generic Integration**
   - Align collection and tensor ops with AR-0018.
   - Expand signatures to use instantiated generic types.

## Open Questions

- Exact signed return type for `compare` and its ordering semantics.
- Whether `render(context)` should accept a structured context object or a minimal handle.

## Next Steps

- Draft ERs for each phase starting with core primitives and Conch integration.
- Define a minimal execution/binding layer for operation implementations.
- Decide standard semantics for `compare`, `split`, and encoding behaviors.
