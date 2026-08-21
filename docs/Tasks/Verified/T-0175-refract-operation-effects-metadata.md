---
Legacy-ID: ER-0079
GitHub-Issue: #287
Source-Path: docs/ER/ER-0079-Refract-Operation-Effects-Metadata.md
---

# T-0175 — Refract Operation Effects Metadata

## Task Metadata

- Task ID: T-0175
- Legacy ID: ER-0079
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #287
- AR Dependencies: -
- Date Requested: 2026-03-19
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0079-Refract-Operation-Effects-Metadata.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #287
---

# ER-0079 — Refract Operation Effects Metadata

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0079
- Title: Refract Operation Effects Metadata
- Status: Verified
- Date: 2026-03-19
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Operation definitions are persisted today, but their side effects and behavioral metadata are not represented as first-class reflection data.
- Background / constraints: AR-0023 identifies operation effects metadata as part of the remaining extended reflection profile. Conduit, Conch, and future policy or planning surfaces need authoritative persisted effect metadata rather than inferred behavior.

## Goals

- Add persisted operation-effects metadata to Refract operation definitions.
- Keep effect metadata queryable for dispatch, introspection, and later policy integration.
- Preserve deterministic schema persistence and bootstrap behavior.

## Non-Goals

- Full permission or sandbox policy enforcement.
- Runtime effect analysis or automatic inference from C++ implementations.
- Redesigning the operation registry model.

## Scope

- In scope: schema metadata for operation effect descriptors.
- In scope: persistence, migration, and introspection support for operation effects.
- In scope: Conduit or Conch-facing query surfaces needed to inspect the stored effects.
- Out of scope: field constraints, documentation objects, and broad policy execution semantics.

## Requirements

- Functional: operation definitions can carry zero or more effect descriptors.
- Functional: operation listing and introspection surfaces can return persisted effect metadata.
- Functional: bootstrap and stored definitions preserve the metadata across reloads.
- Non-functional: the first effect model stays explicit and deterministic.

## Proposed Approach

- Summary: extend `OperationDefinition` with a constrained effect descriptor model, persist it through Refract, and expose it through the existing registry and introspection paths so later policy work can build on stored metadata.
- Alternatives considered: deriving effects only from naming conventions or handler-specific code was rejected because it would not make Refract authoritative for operation behavior metadata.

## Acceptance Criteria

- Schema round-trip tests verify persisted operation effect metadata.
- Introspection surfaces expose effect metadata for operations that declare it.
- Existing dispatch behavior remains stable when operations do not declare effects.

## Risks / Open Questions

- Risk: an effect taxonomy that is too broad too early may force unnecessary migration work.
- Question: which initial effect categories must be modeled now to support future capability and service-boundary ERs?

## Dependencies

- Dependency 1: AR-0023 Refract Reflection Profiles.
- Dependency 2: AR-0016 Operations and Dispatch Model.
- Dependency 3: ER-0055 Refract Inheritance and Interface Metadata.

## Implementation Notes

- Notes for implementer: start with a small effect vocabulary that supports introspection and future policy plumbing without forcing a full semantic system.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect an operation that declares effects through Conch or test helpers.
  - confirm operations without effect metadata continue to list and dispatch normally.
```
