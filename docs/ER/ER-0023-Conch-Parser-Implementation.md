---
GitHub-Issue: N/A
---


# ER-0023 — Conch Parser Implementation

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0023
- Title: Conch Parser Implementation
- Status: Proposed
- Date: 2026-02-21
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: Conch needs a grammar-aware parser to support quoted values,
  flexible whitespace, and structured command parsing.
- Background / constraints: Should build on the Parser Core Framework (ER-0022) and follow
  AR-0015 syntax and grammar guidance.

## Goals

- Implement Conch grammar with a typed AST for commands and arguments.
- Migrate `conch.cc` to consume the AST rather than ad-hoc token vectors.

## Non-Goals

- Shell scripting or advanced expressions.
- Execution semantics beyond parsing/validation.

## Scope

- In scope: tokenizer usage, grammar rules, AST mapping, integration points.
- Out of scope: JSON/XML/C++/Python parsing.

## Requirements

- Functional: parse all existing Conch commands with backward compatibility.
- Non-functional: clear parse errors with location.

## Proposed Approach

- Summary: implement Conch grammar on the core parser, add AST nodes for commands,
  integrate parsing in the command loop.
- Alternatives considered: incremental improvements to whitespace splitting (rejected for
  long-term flexibility).

## Acceptance Criteria

- Conch can parse existing commands and quoted values with spaces.
- Parsing errors report line/offset and message.

## Risks / Open Questions

- Risk: subtle compatibility issues with existing input formats.
- Question: should JSON mode be parsed by the core parser or delegated to JSON parser?

## Dependencies

- Dependency 1: ER-0022 Parser Core Framework.

## Implementation Notes

- Notes for implementer: keep command handling minimal; prefer AST-driven validation.

## Verification Plan

- Tests to run: unit tests for Conch grammar; regression tests for existing commands.
- Manual checks: interactive Conch session with quoted values and spacing variants.
