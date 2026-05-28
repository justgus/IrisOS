---
GitHub-Issue: #275
AR-Dependencies: AR-0015, AR-0026
ER-Dependencies: ER-0067
---

# ER-0068 — Batch Execution and Non-Interactive Parser Integration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0068
- Title: Batch Execution and Non-Interactive Parser Integration
- Status: Complete
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0026 Level 4 calls for parser reuse in batch and userland surfaces, but execution remains centered on the interactive shell path.
- Background / constraints: ER-0067 should provide the reusable parser API. This ER connects that API to a non-interactive execution surface without adding full scripting.

## Goals

- Add a non-interactive command execution path that consumes the reusable parser API.
- Preserve existing command semantics for supported command families.
- Make batch errors deterministic and reportable.

## Non-Goals

- Full shell scripting.
- Conditionals, loops, variables, or expression evaluation.
- Background task orchestration beyond existing command behavior.

## Scope

- In scope: batch input parsing, ordered command execution, error reporting, and tests.
- Out of scope: new grammar constructs and interactive shell UI changes.

## Requirements

- Functional: a caller can submit command text through the non-interactive path.
- Functional: supported commands execute in input order.
- Functional: parse and execution failures identify the failing command.
- Non-functional: interactive command behavior remains unchanged.

## Proposed Approach

- Summary: add a thin batch execution adapter over the reusable parser API and existing command handlers.
- Alternatives considered: building a separate batch interpreter was rejected because Level 4 requires the same command grammar surface.

## Acceptance Criteria

- Tests verify successful ordered execution of multiple simple commands.
- Tests verify parse failures stop or report according to the accepted batch policy.
- Tests verify command execution errors are surfaced deterministically.

## Risks / Open Questions

- Risk: stop-on-first-error versus continue-on-error needs an explicit policy.
- Question: should batch execution return partial results for commands completed before a failure?

## Dependencies

- Dependency 1: ER-0067 Reusable Conch Grammar API.

## Implementation Notes

- Notes for implementer: do not introduce scripting constructs in this ER.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - run a small non-interactive command batch and inspect ordered results.
