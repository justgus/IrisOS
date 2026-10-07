---
GitHub-Issue: N/A
AR-Dependencies: AR-0001
ER-Dependencies: ER-0081
---

# ER-0082 — Refract Lookup NotFound Semantics

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass.

## ER Metadata

- ER ID: ER-0082
- Title: Refract Lookup NotFound Semantics
- Status: Approved
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Refract schema and generic-instance lookups currently represent a missing definition or instance as a successful `std::optional<T>` value, contrary to AR-0001's typed `NotFound` contract.
- Background / constraints: ER-0081 changes Referee object lookup to return `Result<T>` with typed `NotFound`; Refract must propagate that distinction while preserving malformed-record and storage errors.

## Goals

- Align public Refract definition and generic-instance lookup methods with AR-0001.
- Update callers that intentionally register a definition when no current definition exists.

## Non-Goals

- Change schema identity, version selection, inheritance, generic identity, or serialized metadata.
- Change private optional metadata such as an absent migration-hook property.

## Scope

- In scope: `get_definition_by_id`, `get_definition_by_type`, `get_latest_definition_by_type`, `get_instance_by_type`, and `ScopedTypeRegistry::find`, plus their callers and tests.
- Out of scope: public APIs outside Refract and optional values that represent optional schema properties.

## Requirements

- Functional: each lookup returns its record on success and `ErrorCode::NotFound` when the requested definition or instance does not exist.
- Functional: a stored object that is not a type definition remains distinguishable from a missing object.
- Functional: `resolve_or_register` treats only `NotFound` as a cache miss and propagates all other errors.
- Non-functional: existing successful version and type selection behavior remains unchanged.

## Proposed Approach

- Summary: return non-optional values in `Result<T>` from the named lookup APIs, map absence to `ErrorCode::NotFound`, preserve other errors, and migrate callers and tests after ER-0081.
- Alternatives considered: accepting nested optionals as intentional probes was rejected for these named object and type lookups because AR-0001 explicitly requires `NotFound` as an error code.

## Acceptance Criteria

- Tests prove missing IDs, type IDs, latest definitions, and generic instances return `ErrorCode::NotFound`.
- Tests prove successful lookups preserve the expected definition or instance.
- Tests prove `resolve_or_register` registers only after `NotFound` and propagates storage or corruption failures.
- Existing schema, bootstrap, generic-type, and Conch authoring tests pass.

## Risks / Open Questions

- Risk: bootstrap and authoring code use missing-definition results as a normal branch and must not swallow non-NotFound errors.
- Question: none; AR-0001 determines lookup absence semantics.

## Dependencies

- Dependency 1: ER-0081 Referee Object Lookup NotFound Semantics.
- Dependency 2: AR-0001 Expected-Style Error Model.

## Implementation Notes

- Do not convert optional migration-hook fields or unrelated optional metadata.
- Preserve typed error codes when propagating failures from Referee.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_refract_registry test_refract_bootstrap test_conch_authoring`
  - `make check`
- Manual checks: none; focused tests exercise the Refract API boundary.

### Live Pass (Human)

- Required: No
- If no, reason: Unit and integration tests fully cover this non-visual registry API contract.
- Build / environment / target: N/A
- Steps: N/A
- Expected observations: N/A
- Observed results: N/A
- Result: Not Required
- Performed by: N/A
- Date: N/A
