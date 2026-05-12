---
GitHub-Issue: N/A
---

# ER-0078 — Refract Constraints and Validation Metadata

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0078
- Title: Refract Constraints and Validation Metadata
- Status: Complete
- Date: 2026-03-19
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Refract now persists authoritative inheritance and interface metadata, but it still cannot persist field or relationship constraints as first-class reflection data.
- Background / constraints: AR-0023 identifies field constraints and relationship constraints as remaining v2 reflection items. The existing schema registry and Conch authoring flows need deterministic persisted metadata before validation hooks can rely on Refract as the source of truth.

## Goals

- Persist field-level and relationship-level constraint metadata in Refract definitions.
- Add deterministic encode/decode and migration behavior for constraint metadata.
- Provide validation hooks that can consume stored constraints during schema or object-facing operations.

## Non-Goals

- A full symbolic constraint language.
- Policy enforcement beyond schema and relationship validation surfaces.
- Solving every domain-specific constraint model in one ER.

## Scope

- In scope: schema metadata structures for field and relationship constraints.
- In scope: persistence, bootstrap, migration, and introspection support for the new metadata.
- In scope: validation hooks for the supported constraint forms at relevant authoring or registration boundaries.
- Out of scope: operation effects, documentation objects, and broad policy metadata.

## Requirements

- Functional: field definitions can carry zero or more persisted constraints.
- Functional: relationship specifications can carry zero or more persisted constraints.
- Functional: Conch or other introspection surfaces can display the stored constraint metadata.
- Non-functional: constraint persistence and validation behavior remain deterministic across bootstrap and reload.

## Proposed Approach

- Summary: extend the Refract schema model with explicit constraint descriptors, persist them through the registry, and wire the first validation hooks to consume the stored metadata rather than ad hoc rules.
- Alternatives considered: leaving constraints as informal comments or one-off validation logic was rejected because it keeps Refract incomplete as the system of record for reflection.

## Acceptance Criteria

- Schema round-trip tests verify persisted field and relationship constraints.
- Validation tests show supported constraints are enforced through stored metadata.
- Introspection surfaces expose constraint metadata for relevant definitions.

## Risks / Open Questions

- Risk: over-designing the first constraint representation could slow down delivery or create migration churn.
- Question: which constraint kinds are mandatory in v1 beyond requiredness, cardinality, and simple structural checks?

## Dependencies

- Dependency 1: AR-0023 Refract Reflection Profiles.
- Dependency 2: ER-0037 Refract-Native Schema Registry Migration.
- Dependency 3: ER-0055 Refract Inheritance and Interface Metadata.

## Implementation Notes

- Notes for implementer: keep the first constraint model explicit and narrow; prefer a small stable descriptor set over a general-purpose rule engine.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect a type with field and relationship constraints using Conch introspection.
  - verify supported constraint failures surface clear validation errors.
