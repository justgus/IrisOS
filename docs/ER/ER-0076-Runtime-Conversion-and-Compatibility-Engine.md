---
GitHub-Issue: #284
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075
---

# ER-0076 — Runtime Conversion and Compatibility Engine

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Outside an Autonomous AR Batch, only the System Engineer may mark an ER as Verified. In this batch, Codex may do so only after the workflow's independent review and validation gates pass.

## ER Metadata

- ER ID: ER-0076
- Title: Runtime Conversion and Compatibility Engine
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Caliper unit metadata is useful for introspection, but AR-0019 requires deterministic runtime conversion and compatibility evaluation.
- Background / constraints: ER-0075 should provide the expanded catalog needed by the conversion engine.

## Goals

- Add runtime unit compatibility checks based on dimensions.
- Add deterministic conversion chains between compatible units.
- Support scale and offset conversions where the catalog defines them.

## Non-Goals

- Automatic symbolic simplification beyond explicit metadata.
- Domain-specific conversion catalogs.
- Conch command surfaces.

## Scope

- In scope: conversion API, compatibility checks, conversion-chain tests, and failure behavior.
- Out of scope: UI/Conch commands and user-defined catalog overrides unless needed by accepted catalog structure.

## Requirements

- Functional: compatible units can be converted deterministically.
- Functional: incompatible dimensions are rejected.
- Functional: offset conversions such as temperature are handled explicitly.
- Non-functional: conversion results are stable across reloads.

## Proposed Approach

- Summary: implement a conversion engine over the canonical catalog with dimension-based compatibility and explicit chain resolution.
- Alternatives considered: relying on ad hoc per-unit conversion code was rejected because it would bypass Caliper metadata.

## Acceptance Criteria

- Tests verify direct and multi-link conversions, including deterministic results after closing and reopening the same store.
- Tests verify unknown source/target units, incompatible dimensions, malformed decimal and exponent inputs, and missing, cyclic, or dimension-mismatched conversion links return stable errors.
- Tests verify scale factors must be finite and strictly positive, while offsets must be finite and may be negative, zero, or positive. Arithmetic overflow and supported conversion-range boundaries produce deterministic results or stable errors according to the System Engineer's ruling #10; these tests do not select an arithmetic domain.
- Tests verify offset conversions, including Fahrenheit/Celsius, behave deterministically and round only once at the public binary64 result boundary using direct IEEE-754 binary64 round-to-nearest, ties-to-even from the exact rational result. The conversion must not route through `long double` or another platform-dependent intermediate.
- Tests exercise exact halfway values on either side of a representable result and any subnormal/overflow boundaries included in the supported domain selected by ruling #10.
- Candidate exact rounding vectors for `test_refract_bootstrap`, included only if the selected arithmetic domain supports them:
  - `1 + 2^-53 - 2^-55` -> `0x3ff0000000000000`; `1 + 2^-53` (tie) -> `0x3ff0000000000000`; `1 + 2^-53 + 2^-55` -> `0x3ff0000000000001`.
  - `2^-1076` -> `0x0000000000000000`; `2^-1075` (tie) -> `0x0000000000000000`; `3 * 2^-1076` -> `0x0000000000000001`.
  - `(2^54 - 3) / 2^1076` -> `0x000fffffffffffff`; `(2^54 - 2) / 2^1076` (tie) -> `0x0010000000000000`; `(2^54 - 1) / 2^1076` -> `0x0010000000000000`.
  - `2^1024 - 2^970 - 2^968` -> `0x7fefffffffffffff`; `2^1024 - 2^970` (tie) and `2^1024 - 2^970 + 2^968` -> the documented stable finite-range error.

## Risks / Open Questions

- Risk (pending ruling #10): checked rational intermediates may overflow on valid catalog chains. The selected arithmetic domain must be stated explicitly; do not infer arbitrary precision or claim full-catalog conversion coverage from these candidate vectors.
- Resolved during batch planning: keep the existing Caliper `scale` and `offset` fields as binary64; changing the persisted numeric schema is outside AR-0019's accepted direction. The conversion engine interprets each factor's shortest round-trip decimal representation as an exact rational intermediate, performs checked exact chain arithmetic, and rounds once to binary64 at the public result boundary.

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.

## Implementation Notes

- Conversion follows each unit's explicit base-unit chain and composes scale and offset transforms.
- Decimal input and the shortest round-trip decimal forms of stored binary64 factors are represented as reduced rational values. Arithmetic range is provisional pending ruling #10: either exact in-tree arbitrary-precision intermediates across the supported catalog domain, or checked signed 64-bit arithmetic with an explicitly bounded conversion domain. Do not approximate when intermediate arithmetic exceeds the selected domain.
- The public result is IEEE-754 binary64 rounded directly from the exact rational result using round-to-nearest, ties-to-even. Intermediate overflow outside the selected domain returns a deterministic error. Candidate vectors outside the selected arithmetic range are not required; reconcile the final test set with ruling #10 before approval.
- No new dependency is required.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_refract_bootstrap'`
  - `make check`
  - `git diff --check`
- Manual checks:
  - convert representative length, mass, time, and temperature values.

## Completion Record

- Prior implementation is merged; batch planning identified acceptance-test gaps that must be closed before this ER can return to Complete. System Engineer verification remains pending.
- Validation: `make check` passed all 29 test programs on 2026-10-05.

### Live Pass (Human)

- Required: No
- If no, reason: Conversion and error behavior is fully observable through the runtime API tests and requires no target-specific environment or human-only interaction.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
