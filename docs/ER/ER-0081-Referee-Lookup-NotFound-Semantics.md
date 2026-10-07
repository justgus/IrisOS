---
GitHub-Issue: N/A
AR-Dependencies: AR-0001
---

# ER-0081 — Referee Object Lookup NotFound Semantics

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass.

## ER Metadata

- ER ID: ER-0081
- Title: Referee Object Lookup NotFound Semantics
- Status: Approved
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Referee object reads expose absence as `Result<std::optional<ObjectRecord>>`, although AR-0001 requires missing objects to be reported with `ErrorCode::NotFound`.
- Background / constraints: `ErrorCode::NotFound` already exists. The `Result<T>` implementation may continue to use `std::optional<T>` internally; public Referee reads should return `Result<ObjectRecord>` and distinguish a missing object from operational errors.

## Goals

- Make object lookup absence an explicit typed error at the Referee API boundary.
- Update callers and tests to handle missing objects without confusing absence with storage failure.

## Non-Goals

- Change object identity, versioning, serialization, persistence, or successful lookup behavior.
- Migrate Refract, CEO, service, or Vizier lookup APIs; those are separate ERs.

## Scope

- In scope: `SqliteStore::get_object` and `SqliteStore::get_latest`, their consumers, and focused Referee tests.
- Out of scope: list queries, edge queries, and optional fields that are not object lookup results.

## Requirements

- Functional: `get_object` returns the record on success and `ErrorCode::NotFound` for an absent `ObjectRef`.
- Functional: `get_latest` returns the latest record on success and `ErrorCode::NotFound` for an absent `ObjectID`.
- Functional: closed-store and persistence failures remain errors and are never reported as `NotFound`.
- Non-functional: successful reads preserve current record contents and ordering.

## Proposed Approach

- Summary: change both public return types to `Result<ObjectRecord>`, return typed `NotFound` at missing lookup points, and update every compiler-identified caller to propagate or handle that error explicitly.
- Alternatives considered: retaining nested optionals was rejected because it preserves the lookup ambiguity identified by AR-0001.

## Acceptance Criteria

- Tests prove successful reads return the expected record through the direct `Result<T>` value.
- Tests prove a missing exact reference and a missing latest-object ID both carry `ErrorCode::NotFound`.
- Tests prove a closed-store read does not carry `ErrorCode::NotFound`.
- All consumers of these APIs compile and preserve their existing behavior for present records.

## Risks / Open Questions

- Risk: many Conch and Refract call sites currently branch on the inner optional and need careful error propagation.
- Question: none; AR-0001 determines missing-object semantics.

## Dependencies

- Dependency 1: AR-0001 Expected-Style Error Model.

## Implementation Notes

- Do not convert unrelated `std::optional` values or empty collection results.
- Preserve underlying error codes when propagating a failed `Result`.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_referee_core`
  - `make check`
- Manual checks: none; focused tests exercise the public Referee API boundary.

### Live Pass (Human)

- Required: No
- If no, reason: Unit and integration tests fully cover this non-visual storage API contract.
- Build / environment / target: N/A
- Steps: N/A
- Expected observations: N/A
- Observed results: N/A
- Result: Not Required
- Performed by: N/A
- Date: N/A
