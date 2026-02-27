---
GitHub-Issue: #TBD
---

# ER-0040 — Generic Type System Core

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0040
- Title: Generic Type System Core
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0018 requires a first-class generic type system with explicit TypeID encoding.
- Background / constraints: must remain deterministic and introspectable across processes.

## Goals

- Implement GenericInstance schema and canonical TypeID encoding.
- Support type/value/variadic parameters with validation rules.

## Non-Goals

- Full dependent type system.
- Complex type inference across expressions.

## Scope

- In scope: schema metadata, instantiation rules, and TypeID encoding.
- Out of scope: compiler-grade inference or symbolic constraint solving.

## Requirements

- Functional: generic types can be instantiated and encoded deterministically.
- Non-functional: TypeIDs are stable and human-readable by default.

## Proposed Approach

- Summary: define GenericInstance as a Refract schema object and implement encoding rules.
- Alternatives considered: hashed TypeIDs by default (rejected).

## Acceptance Criteria

- Generic instances are stored and resolved deterministically.
- TypeID encoding is explicit and stable across processes.

## Risks / Open Questions

- Risk: value parameter normalization rules may be ambiguous.
- Question: which constraints are mandatory in v1?

## Dependencies

- Dependency 1: AR-0018 Generic Type System.
- Dependency 2: ER-0037 Refract-Native Schema Registry Migration.

## Implementation Notes

- Notes for implementer: keep encoding rules fully specified in code and docs.

## Verification Plan

- Tests to run: deterministic instantiation and TypeID tests.
- Manual checks: compare TypeID derivations across runs.
