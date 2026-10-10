---
GitHub-Issue: #285
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075, ER-0076
---

# ER-0077 — Conch Conversion and Inspection Commands

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Outside an Autonomous AR Batch, only the System Engineer may mark an ER as Verified. In this batch, Codex may do so only after the workflow's independent review and validation gates pass.

## ER Metadata

- ER ID: ER-0077
- Title: Conch Conversion and Inspection Commands
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Caliper catalog and conversion behavior need a user-facing Conch surface for listing, lookup, and conversion.
- Background / constraints: ER-0075 and ER-0076 should provide the catalog and runtime conversion engine.

## Goals

- Add Conch commands for unit listing and lookup.
- Add Conch commands for converting values between compatible units.
- Surface deterministic errors for unknown units and incompatible dimensions.

## Non-Goals

- Full scripting integration.
- Domain-specific catalog authoring UI.
- Rich table rendering beyond existing Conch output conventions.

## Scope

- In scope: command parsing, execution, output formatting, and tests.
- Out of scope: new catalog data and conversion engine internals.

## Requirements

- Functional: users can list units and inspect unit metadata.
- Functional: users can convert values between compatible units.
- Functional: unknown units and incompatible conversions produce clear errors.
- Non-functional: commands follow existing Conch naming and error conventions.

## Proposed Approach

- Summary: add thin Conch command wrappers over the Caliper catalog and conversion APIs.
- Alternatives considered: exposing conversion only through tests or libraries was rejected because AR-0019 calls for Conch and runtime API access.

## Acceptance Criteria

- Command tests verify unit listing and inspection by both symbol and name, plus stable errors for unknown units and malformed command arity.
- Command tests verify successful conversion output, including the numeric result and target symbol, with optional dimension output when requested.
- Tests verify unknown units, incompatible dimensions, and invalid options produce deterministic command errors.

## Risks / Open Questions

- Risk: command names may conflict with future parser grammar expansion.
- Resolved during batch planning by the existing merged command contract: conversion prints the numeric result and target symbol by default; `--dimension` opts into dimension metadata, while `inspect` reports unit metadata. ER-0077 preserves and tests this output contract.

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.
- Dependency 2: ER-0076 Runtime Conversion and Compatibility Engine.

## Implementation Notes

- Commands are available as `caliper list`, `caliper inspect <symbol-or-name>`, and `caliper convert <value> <from-unit> <to-unit> [--dimension]`.
- Conversion output includes the converted value and target symbol by default. `--dimension` adds the dimension label.
- Output precision uses the round-trip digit count for binary64; errors follow the existing `error:` convention.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_conch_parser test_conch_authoring'`
  - `make check`
  - `git diff --check`
- Manual checks:
  - list units, inspect a unit, convert compatible units, and attempt an incompatible conversion.

## Completion Record

- Prior implementation is merged; batch planning identified acceptance-test gaps that must be closed before this ER can return to Complete. System Engineer verification remains pending.
- Validation: `make check` passed all 29 test programs on 2026-10-05.

### Live Pass (Human)

- Required: No
- If no, reason: Listing, inspection, conversion output, and error behavior are covered by automated Conch subprocess tests; no target-specific environment or human-only interaction is required.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
