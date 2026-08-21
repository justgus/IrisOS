---
Legacy-ID: ER-0069
GitHub-Issue: #276
Source-Path: docs/ER/ER-0069-Shared-Parser-Regression-Harness.md
---

# T-0165 — Shared Parser Regression Harness

## Task Metadata

- Task ID: T-0165
- Legacy ID: ER-0069
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #276
- AR Dependencies: AR-0015, AR-0026
- Date Requested: 2026-05-28
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0069-Shared-Parser-Regression-Harness.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #276
AR-Dependencies: AR-0015, AR-0026
ER-Dependencies: ER-0067, ER-0068
---

# ER-0069 — Shared Parser Regression Harness

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0069
- Title: Shared Parser Regression Harness
- Status: Verified
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Once parser behavior is shared by shell and non-interactive consumers, regressions need to be caught across both paths.
- Background / constraints: ER-0067 and ER-0068 establish shared parser and batch execution surfaces.

## Goals

- Add shared parser regression fixtures for shell and non-interactive parser use.
- Cover representative command families and parse-error cases.
- Keep the harness easy to extend when grammar levels grow.

## Non-Goals

- New parser features.
- Exhaustive scripting conformance.
- External test dependencies.

## Scope

- In scope: parser fixture format, shared tests, and coverage for success and failure cases.
- Out of scope: command execution semantics beyond what is needed to validate parser reuse.

## Requirements

- Functional: the same fixtures can validate interactive and non-interactive parse behavior.
- Functional: expected AST shape and expected errors can be asserted deterministically.
- Non-functional: tests run under the normal `make check` path.

## Proposed Approach

- Summary: add a small in-repo regression harness that feeds fixture commands through both parser entry points and compares normalized parse results.
- Alternatives considered: relying on ad hoc unit tests was rejected because the parser will have multiple consumers.

## Acceptance Criteria

- Tests verify shared fixtures pass through shell and reusable parser paths.
- Tests verify parse-error fixtures produce consistent diagnostics.
- The harness is documented enough for later ERs to add cases.

## Risks / Open Questions

- Risk: over-specific AST snapshots may make legitimate parser refactors noisy.
- Question: what normalized AST representation should be used for fixture assertions?

## Dependencies

- Dependency 1: ER-0067 Reusable Conch Grammar API.
- Dependency 2: ER-0068 Batch Execution and Non-Interactive Parser Integration.

## Implementation Notes

- Notes for implementer: prefer normalized expectations over raw object dumps.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - add one success fixture and one failure fixture, then confirm both parser paths exercise them.
```
