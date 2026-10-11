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
- Status: Verified
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
- Tests verify scale factors must be finite and strictly positive, while offsets must be finite and may be negative, zero, or positive. The arithmetic domain is checked signed 64-bit rational numerator and positive denominator after reduction; any intermediate outside this domain returns a stable overflow error. Decimal input and catalog factors must be representable in that domain. Conversion results are rounded to finite binary64 values; subnormal and overflow-boundary results are outside this domain.
- Tests verify offset conversions, including Fahrenheit/Celsius, behave deterministically and round only once at the public binary64 result boundary using direct IEEE-754 binary64 round-to-nearest, ties-to-even from the exact rational result. The conversion must not route through `long double` or another platform-dependent intermediate.
- Tests exercise exact halfway values on either side of a representable result. Subnormal and binary64 overflow boundaries are excluded because the selected signed 64-bit rational domain cannot represent them.
- Candidate exact rounding vectors for `test_refract_bootstrap` within the selected domain:
  - `1 + 2^-53 - 2^-55` -> `0x3ff0000000000000`; `1 + 2^-53` (tie) -> `0x3ff0000000000000`; `1 + 2^-53 + 2^-55` -> `0x3ff0000000000001`.

## Risks / Open Questions

- Risk: checked rational intermediates may overflow on catalog chains with large or small scales; these return a stable error rather than an approximation.
- Resolved during batch planning: keep the existing Caliper `scale` and `offset` fields as binary64; changing the persisted numeric schema is outside AR-0019's accepted direction. The conversion engine interprets each factor's shortest round-trip decimal representation as an exact rational intermediate, performs checked exact chain arithmetic, and rounds once to binary64 at the public result boundary.
- Superseded for the Fahrenheit affine transform by the 2026-10-11 AR-0019 ruling: Unit `scale`/`offset` payload fields remain binary64 and unchanged, while immutable catalog v2's Temperature dimension entry carries supplemental exact rational factors for Fahrenheit. Runtime conversion uses those exact factors where present. This also keeps ordinary Fahrenheit conversions within the accepted signed-64-bit rational domain.

## Dependencies

- Dependency 1: ER-0075 Full Caliper Catalog Expansion.

## Implementation Notes

- Conversion follows each unit's explicit base-unit chain and composes scale and offset transforms.
- Decimal input and the shortest round-trip decimal forms of stored binary64 factors are represented as reduced rationals with signed 64-bit numerator and positive signed 64-bit denominator. Every arithmetic intermediate is checked; overflow returns a stable error.
- An exact rational transform in the versioned base catalog takes precedence over binary64 factors for that unit; the current catalog uses this for Fahrenheit-to-kelvin conversion.
- The public result is IEEE-754 binary64 rounded directly from the exact rational result using round-to-nearest, ties-to-even, without a floating-point intermediate. The rational range is many orders of magnitude narrower than binary64, so subnormal and binary64 overflow boundaries are out of scope.
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

- Replaced the platform-dependent `long double` result path with integer-only IEEE-754 binary64 round-to-nearest, ties-to-even. The signed 64-bit rational domain is explicit; decimal parsing reduces decimal powers before enforcing the domain, including `INT64_MIN`.
- Added exact rounding tie vectors, reducible long-decimal and signed-boundary vectors, stable error assertions for malformed values and invalid conversion graphs/factors, root-factor validation, and conversion-after-store-reopen bit stability.
- Independent review examined rounding, parser bounds, conversion-chain failures, and root factors. Findings on reducible decimals and root-unit validation were fixed and the reviewer confirmed closure. No independent-review finding remains open.
- A follow-up absolute-zero boundary probe exposed a negative-decimal sign regression in the denominator-reduction path. Fixed it and added negative reducible decimal, `INT64_MIN`, and exact `-273.15 °C -> 0 K` bit-pattern checks; the updated focused and full suites pass.
- Validation on macOS `aarch64-apple-darwin27.2.0`: `make -j2 CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` passed; `make -C tests test_refract_bootstrap CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` plus `DYLD_LIBRARY_PATH="$PWD/src/.libs" ./tests/.libs/test_refract_bootstrap` passed (14/14); `DYLD_LIBRARY_PATH="$PWD/src/.libs" TMPDIR=/private/tmp make -j1 check CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` passed (30/30); `git diff --check` passed. The flags suppress existing Xcode unused-member diagnostics. `make check` modified the tracked `tests/test_referee_core` executable; it was restored. No Autotools inputs changed and no regeneration was needed.
- Verified after PR #347 merged and CI run 38166633486 passed.

### Independent Live Pass

- Required: No
- If no, reason: Conversion and error behavior is fully observable through the runtime API tests and requires no target-specific environment or human-only interaction.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable


## Final Batch Verification

- Verified after local validation, independent review where required, PR merge, and PR #347; CI run 38166633486 passing its required CI checks. The detailed evidence is recorded in `docs/Plans/AR-Batch-Progress.md`.
