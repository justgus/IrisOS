---
GitHub-Issue: #110
---


# ER-0029 — Conduit Dispatch Engine

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0029
- Title: Conduit Dispatch Engine
- Status: Verified
- Date: 2026-02-25
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: Operations require deterministic overload resolution and polymorphic
  dispatch as described in AR-0016.
- Background / constraints: Must honor class/object scopes and inherited operations.

## Goals

- Implement dispatch algorithm with clear ambiguity errors.
- Support override precedence and inherited fallback.

## Non-Goals

- Multi-dispatch across unrelated types.
- Implicit conversions beyond defined rules.

## Scope

- In scope: overload selection, specificity rules, error reporting.
- Out of scope: execution runtime and task scheduling.

## Requirements

- Functional: dispatch chooses the most specific matching operation.
- Non-functional: deterministic behavior and traceable errors.

## Proposed Approach

- Summary: implement a dispatch resolver that scores candidates based on type specificity
  and optionality.
- Alternatives considered: user-defined resolution hooks (defer).

## Acceptance Criteria

- Ambiguous calls produce a clear error listing candidates.
- Overrides in derived types take precedence.

## Risks / Open Questions

- Risk: corner cases with optional parameters and overloads.
- Question: should there be a tie-breaking rule beyond specificity?

## Dependencies

- Dependency 1: ER-0028 Conduit Operation Model.

## Implementation Notes

- Notes for implementer: keep matching rules explicit and documented.

## Verification Plan

- Tests to run: dispatch resolution unit tests with inheritance and overload sets.
- Manual checks: run sample `call` validations in Conch.
