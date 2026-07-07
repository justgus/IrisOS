---
Legacy-ID: ER-0055
GitHub-Issue: #262
Source-Path: docs/ER/ER-0055-Refract-Inheritance-and-Interface-Metadata.md
---

# T-0151 — Refract Inheritance and Interface Metadata

## Task Metadata

- Task ID: T-0151
- Legacy ID: ER-0055
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #262
- AR Dependencies: -
- Date Requested: 2026-03-14
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0055-Refract-Inheritance-and-Interface-Metadata.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #262
---

# ER-0055 — Refract Inheritance and Interface Metadata

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0055
- Title: Refract Inheritance and Interface Metadata
- Status: Verified
- Date: 2026-03-14
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Conduit dispatch and type introspection still depend on external inheritance resolver callbacks instead of authoritative persisted relationships in Refract.
- Background / constraints: AR-0007 defines a richer reflection graph, and AR-0023 narrows the first implementation wave to persisted inheritance and interface metadata.

## Goals

- Persist base-type and implemented-interface metadata in Refract definitions.
- Make schema registry and dispatch logic capable of reading authoritative inheritance information from stored metadata.
- Preserve deterministic bootstrap and lookup behavior.

## Non-Goals

- Full constraint language support.
- Documentation object support.
- Broad policy metadata beyond capability fields already present.

## Scope

- In scope: `TypeDefinition` metadata changes for inheritance and interfaces.
- In scope: schema registry encode/decode support and bootstrap updates.
- In scope: dispatch and operation listing updates to consume authoritative stored relationships.
- In scope: migration handling for existing definitions that do not yet carry the new metadata.
- Out of scope: effects, documentation, and broader reflection extensions.

## Requirements

- Functional: type definitions can declare zero or more base types.
- Functional: type definitions can declare zero or more implemented interfaces.
- Functional: operation listing and dispatch can consume stored inheritance metadata without requiring ad-hoc resolver callbacks for the common path.
- Functional: `show type` and related introspection surfaces display the new metadata.
- Non-functional: persistence, bootstrap, and migration behavior remain deterministic.

## Proposed Approach

- Summary: extend Refract type metadata to store inheritance and interface relationships directly, then update the registry and dispatch code to use those relationships as the authoritative source.
- Alternatives considered: keeping inheritance entirely external to Refract was rejected because it leaves the reflection graph incomplete and makes dispatch behavior harder to reason about.

## Acceptance Criteria

- Schema round-trip tests verify persisted inheritance and interface metadata.
- Dispatch tests succeed using stored relationships rather than test-only resolver wiring for the common path.
- Conch introspection surfaces expose base and interface metadata for relevant types.

## Risks / Open Questions

- Risk: schema migration for existing definitions may be easy to get wrong if bootstrap and persisted records diverge.
- Question: should interfaces be represented only as tagged type references in this ER, with richer interface semantics deferred?

## Dependencies

- Dependency 1: AR-0023 Refract Reflection Profiles.
- Dependency 2: AR-0016 Operations and Dispatch Model.
- Dependency 3: ER-0037 Refract-Native Schema Registry Migration.

## Implementation Notes

- Notes for implementer: keep migration behavior explicit and testable; do not rely on silent defaults when old metadata is absent.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - `show type` for a derived type displays base or interface metadata.
  - dispatch and operation listing behavior remains stable after persistence round-trips.
```
