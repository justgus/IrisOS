---
GitHub-Issue: #108
---


# ER-0027 — Python Parser

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0027
- Title: Python Parser
- Status: Verified
- Date: 2026-02-24
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: A Python parser could enable analysis or ingestion of scripts.
- Background / constraints: Python syntax and indentation rules require careful handling.

## Goals

- Define a focused Python parsing scope (subset or specific use cases).
- Integrate indentation-aware parsing on the core parser framework.

## Non-Goals

- Full Python language support in v1.
- Runtime execution or semantic evaluation.

## Scope

- In scope: subset grammar with indentation tokens, statements, and expressions as needed.
- Out of scope: full Python semantics, imports, runtime execution.

## Requirements

- Functional: parse the defined subset and enforce indentation rules.
- Non-functional: clear parse errors with line/column information.

## Proposed Approach

- Summary: extend the core parser with indentation tokenization; implement a constrained
  Python grammar.
- Alternatives considered: using a third-party Python parser (defer until scope is clear).

## Acceptance Criteria

- Parser handles the defined subset and reports indentation errors correctly.
- Tests cover indentation edge cases.

## Risks / Open Questions

- Risk: indentation handling complicates the core tokenizer.
- Question: what Python subset is actually needed for IrisOS?

## Dependencies

- Dependency 1: ER-0022 Parser Core Framework.

## Implementation Notes

- Notes for implementer: isolate indentation handling to avoid impacting other grammars.

## Verification Plan

- Tests to run: unit tests for indentation and grammar subset.
- Manual checks: parse a small Python snippet with blocks.
