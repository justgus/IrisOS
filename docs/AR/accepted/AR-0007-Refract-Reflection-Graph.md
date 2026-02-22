---
GitHub-Issue: #71
---

# AR-0007 — Refract Reflection Graph (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS requires reflection that is persistent, versioned, queryable, and security-aware. Reflection
must be usable by humans and machines, and must be part of the object graph (not just compiled
metadata).

## Recommendation

Define **Refract** as a first-class reflection graph stored in Referee. Reflection data is composed
of objects that describe types, operations, fields, relationships, and documentation. All Refract
entities are objects and can be versioned, queried, and governed by permissions.

### Core Refract Object Types (Minimum)

- Type
- TypeID
- name / namespace
- version
- base types (inheritance)
- implemented interfaces

- Field
  - name
  - type
  - required/optional
  - default
  - constraints

- Operation
  - name
  - signature (args → return)
  - side-effects (pure / mutating / spawning tasks)
  - permissions required

- Signature
  - ordered parameters
  - types
  - variadic / optional

- RelationshipSpec
  - role name
  - cardinality
  - constraints

- Documentation
  - human text
  - examples

## Notes

Refract is not "C++ RTTI but nicer." It is the authoritative, persistent reflection graph used by
shells, services, and tools. Refract objects live in Referee and are part of the system's object
model.
