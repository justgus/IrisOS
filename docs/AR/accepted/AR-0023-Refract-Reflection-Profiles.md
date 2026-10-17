---
GitHub-Issue: #291
ER-Dependencies: ER-0055, ER-0078, ER-0079, ER-0080, ER-0103
---

# AR-0023 — Refract Reflection Profiles

- Status: Implemented
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

- No remaining work in the accepted profile scope; see the System Engineer ruling below.

## Implementation Progress

- The 2026-10-13 source audit found persisted inheritance/interfaces, field and relationship
  constraints, operation effects, and documentation/examples in `src/refract/schema_registry.*`.
  The previously linked ERs (ER-0055/0078/0079/0080) are Verified. The corresponding registry,
  bootstrap, and Conch tests pass in fresh `TMPDIR=/private/tmp make check` (31/31, outside the sandbox).
- Operations also persist and expose `required_capabilities`. Whether this satisfies the AR's
  extended policy metadata requirement was accepted by the System Engineer on 2026-10-15.
  All listed dependency ERs are Verified; the reconciliation below records completion.

### Existing Metadata Repair (2026-10-15)

- Independent review found that malformed capabilities containers can decode as empty requirements.
  Batch-approved ER-0103 validates the already-defined array-of-strings format and tests file-backed
  capability persistence and inherited operation listing. This repair does not define a broader
  policy model; the System Engineer separately accepted the existing metadata scope below.

### System Engineer Ruling and Reconciliation — 2026-10-15

- The System Engineer confirmed that persisted operation `required_capabilities` satisfies
  this batch’s extended policy metadata requirement. Earlier scope questions are resolved.
- ER-0055/0078/0079/0080 and ER-0103 are Verified. The persisted inheritance/interfaces,
  constraints, effects, documentation, and capability metadata evidence above realizes the
  selected profiles. ER-0103 independent review and exact-head CI passed. Status: Implemented.
