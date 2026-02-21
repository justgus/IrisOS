---
GitHub-Issue: N/A
---

# AR-0016 — Operations and Dispatch Model (Proposal)

- Status: Proposed
- Date: 2026-02-21
- Owners: Mike

## Context

IrisOS needs a consistent way to define and invoke Operations as the system's core function model.
Operations must be tied to types, support overloading and polymorphism, and distinguish between
class-level (static) and object-level (instance) behavior. This is expected to be a large subsystem
and should integrate with Refract (reflection), Referee (object store), and Conch (shell).

## Recommendation

Define a formal Operations and Dispatch Model that:

- Treats Operations as first-class Refract entities owned by a Type.
- Supports overload resolution via explicit signatures.
- Supports polymorphism via type relationships (inheritance or interface conformance).
- Separates class-level (static) operations from object-level (instance) operations.
- Provides a deterministic dispatch algorithm and validation rules.

### Naming

Proposed subsystem name: **Dispatch** (preferred), or **Ops** if we want to keep it short.
The name should emphasize resolution + invocation rather than storage.

## Concepts

- **OperationDefinition**: describes name, scope (class/object), signature, effects, and permissions.
- **Signature**: ordered parameters with type IDs, optional/variadic markers, and return type.
- **Scope**:
  - **Class**: callable without an instance (e.g., `Type::create(...)`)
  - **Object**: callable on an instance (e.g., `obj.update(...)`)
- **Overload Set**: multiple OperationDefinitions with the same name and scope, distinguished by signature.
- **Dispatch**: algorithm to choose the best overload given actual arguments and the target type.
- **Polymorphism**: allow a call to resolve against a base/interface definition when invoked on a derived type.

## Goals

- Make Operations discoverable and introspectable in Refract.
- Ensure deterministic overload resolution and clear ambiguity errors.
- Provide consistent behavior across Conch, APIs, and internal execution.
- Allow future extensions (permissions, async/task spawning, side-effect metadata).

## Non-Goals (v1)

- Full dynamic language semantics.
- Complex multiple-dispatch across unrelated types.
- User-defined implicit conversions beyond simple numeric widening (if any).

## Proposed Model (Broad Strokes)

### Definition

- Each Type owns a list of OperationDefinitions.
- Each OperationDefinition includes:
  - name
  - scope: `class` or `object`
  - signature (params + return type)
  - effects: `pure`, `mutating`, `spawns_task`, etc.
  - permissions (future)

### Dispatch Rules (Sketch)

1. Select operations by name and scope.
2. Filter by arity and argument optionality.
3. Match parameter types by:
   - exact type
   - subtype/interface conformance
   - optional: numeric widening rules (if defined)
4. If multiple matches remain:
   - choose the most specific type match
   - if still ambiguous, error

### Polymorphism

- If a derived type doesn't define an operation, dispatch may fall back to base/interface operations.
- If a derived type defines an override with identical signature, it wins.

## Open Questions

- What type system relationships are authoritative for polymorphism (inheritance, interfaces, both)?
- Should class-level operations be allowed to be inherited/overridden?
- Do we support optional implicit conversions in v1?

## Next Steps

- Define the Refract schema objects for OperationDefinition and Signature in detail.
- Draft an ER set that breaks implementation into:
  - Core dispatch engine
  - Conch integration
  - Referee storage + Refract linkage
