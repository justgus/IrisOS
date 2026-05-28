---
GitHub-Issue: #291
ER-Dependencies: ER-0055, ER-0078, ER-0079, ER-0080
---

# AR-0023 — Refract Reflection Profiles

- Status: Accepted
- Date: 2026-03-14
- Owners: Mike

## Context

AR-0007 defines Refract as a rich, authoritative reflection graph. The current implementation
already persists types, fields, signatures, operations, relationships, generic instances, and
renderer metadata, but it does not yet persist authoritative inheritance, interfaces, constraints,
operation effects, documentation objects, or broader policy metadata.

The accepted architecture and the implemented reflection model are therefore describing different
levels of Refract maturity.

## Recommendation

Define explicit Refract reflection profiles so that near-term implementation and verification can be
stated precisely.

### Refract v1 Core Profile

- Type definitions
- Field definitions
- Signature definitions
- Operation definitions
- Relationship specifications
- Generic instance records
- Preferred renderer metadata
- Definition supersession and migration hooks

### Refract v2 Extended Profile

- Authoritative base-type relationships
- Authoritative implemented-interface relationships
- Field constraints
- Relationship constraints
- Operation effects metadata
- Documentation objects and examples
- Extended policy metadata

## Goals

- Preserve Refract as the system of record for reflection while clarifying maturity levels.
- Allow Conduit and Conch to rely on a documented v1 reflection surface.
- Provide a clean path for completing the richer AR-0007 model later.

## Non-Goals (v1)

- Deferring all reflection improvements until a larger rewrite
- Replacing Refract with compile-time-only metadata
- Solving all validation and policy semantics in one step

## Proposed Model

- AR-0007 remains the umbrella architecture recommendation.
- This follow-on AR defines the profile boundary between implemented core reflection and future
  extended reflection.
- The first implementation wave should add authoritative inheritance metadata to move a key v2 item
  into the near-term roadmap.

## Relationship To Existing ARs

- Refines [AR-0007-Refract-Reflection-Graph.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md)
- Supports [AR-0016-Operations-Dispatch.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0016-Operations-Dispatch.md)

## Next Steps

- Draft ER-0055 for persisted inheritance and interface metadata.
- Follow with ERs for constraints, effects, and documentation.
