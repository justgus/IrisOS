---
GitHub-Issue: N/A
---


# ER-0024 — JSON Parser

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0024
- Title: JSON Parser
- Status: Proposed
- Date: 2026-02-21
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: IrisOS needs a JSON parser for data ingestion and interop.
- Background / constraints: Use the shared Parser Core Framework where sensible, but
  avoid replacing stable existing JSON use unless required.

## Goals

- Provide a JSON parser with a consistent AST/value model.
- Allow reuse in Conch `--json` flows or other tooling.

## Non-Goals

- JSON schema validation.
- Streaming JSON parsing in v1.

## Scope

- In scope: JSON grammar, AST/value mapping, error reporting.
- Out of scope: schema/validation layers, JSON5 extensions.

## Requirements

- Functional: parse standard JSON per RFC 8259.
- Non-functional: clear error messages with location.

## Proposed Approach

- Summary: implement JSON grammar on the Parser Core Framework; map to shared value nodes.
- Alternatives considered: continue to rely on third-party JSON parsing (defer decision
  until integration plan is clearer).

## Acceptance Criteria

- JSON parser accepts standard JSON and rejects invalid syntax with precise errors.
- Parser can be used by at least one IrisOS component.

## Risks / Open Questions

- Risk: duplication with existing JSON usage in the codebase.
- Question: should we migrate existing JSON parsing or keep both?

## Dependencies

- Dependency 1: ER-0022 Parser Core Framework.

## Implementation Notes

- Notes for implementer: avoid new third-party dependencies.

## Verification Plan

- Tests to run: JSON parsing unit tests covering objects, arrays, numbers, strings.
- Manual checks: parse a sample Conch `--json` payload if integrated.
