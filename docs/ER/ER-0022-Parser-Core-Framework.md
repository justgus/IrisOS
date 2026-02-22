---
GitHub-Issue: #103
---


# ER-0022 — Parser Core Framework

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0022
- Title: Parser Core Framework
- Status: Proposed
- Date: 2026-02-21
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: IrisOS needs a reusable parsing framework to support Conch command
  syntax and additional structured parsers (JSON, XML, C++, Python) under a common API.
- Background / constraints: AR-0015 defines the parser direction and grammar goals. The
  framework should be a small, standalone library usable across subsystems.

## Goals

- Provide a tokenizer + parser API with deterministic error reporting.
- Define a shared AST/value model and error types usable by multiple grammars.

## Non-Goals

- Full scripting language features.
- Language-specific semantics beyond parsing.

## Scope

- In scope: token model, parser API, error model, shared AST/value types.
- Out of scope: Conch grammar, JSON/XML/C++/Python grammar specifics.

## Requirements

- Functional: parse input into an AST or structured value tree with offsets and error
  messages.
- Non-functional: deterministic behavior, clear error messages, small API surface.

## Proposed Approach

- Summary: implement a core `Parser`/`Tokenizer` library with reusable primitives and a
  small set of shared node/value types.
- Alternatives considered: using third-party parser generators (rejected for now to keep
  dependencies minimal).

## Acceptance Criteria

- Library compiles and is used by at least one grammar (Conch) without duplicate parsing
  logic.
- Errors include location and message.

## Risks / Open Questions

- Risk: overgeneralizing the AST/value types could hinder language-specific needs.
- Question: do we need streaming/incremental parsing at v1?

## Dependencies

- Dependency 1: AR-0015 Conch Parser and Syntax.

## Implementation Notes

- Notes for implementer: keep the API minimal and stable; avoid new dependencies.

## Verification Plan

- Tests to run: unit tests for tokenizer and core parse primitives.
- Manual checks: parse a small Conch input and confirm errors are readable.
