---
GitHub-Issue: #107
---


# ER-0026 — C++ Parser

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0026
- Title: C++ Parser
- Status: Proposed
- Date: 2026-02-21
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: A C++ parser could support static analysis, code ingestion, or
  metadata extraction for IrisOS.
- Background / constraints: Full C++ parsing is complex; v1 should target a narrow subset
  or specific use cases.

## Goals

- Define a focused C++ parsing scope (subset or metadata extraction).
- Evaluate feasibility of implementing a grammar on the core parser framework.

## Non-Goals

- Full C++20/23 compliance in v1.
- Semantic analysis, type checking, or compilation.

## Scope

- In scope: limited grammar for declarations and types as needed by IrisOS.
- Out of scope: full language support, preprocessing, templates in full generality.

## Requirements

- Functional: parse the defined subset reliably with errors.
- Non-functional: clear scope boundaries; fail fast on unsupported syntax.

## Proposed Approach

- Summary: define a constrained grammar and implement it on the core parser framework.
- Alternatives considered: using an external compiler front-end (defer until scope is
  confirmed).

## Acceptance Criteria

- Parser handles the agreed subset and rejects unsupported constructs clearly.
- Documented scope is enforced by tests.

## Risks / Open Questions

- Risk: scope creep from “subset” toward full C++ complexity.
- Question: what concrete IrisOS use cases require C++ parsing?

## Dependencies

- Dependency 1: ER-0022 Parser Core Framework.

## Implementation Notes

- Notes for implementer: define and document the subset early.

## Verification Plan

- Tests to run: unit tests for the supported subset.
- Manual checks: parse a small header file subset.
