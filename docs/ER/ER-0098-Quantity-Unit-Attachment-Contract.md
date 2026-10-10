---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0033
---

# ER-0098 — Quantity Unit Attachment Contract

## ER Metadata

- ER ID: ER-0098
- Title: Quantity Unit Attachment Contract
- Status: Complete
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 requires units and dimension metadata for quantities. Current quantity schemas expose optional `unit_id`, while ordinary values have no generic unit field. The accepted contract attaches the optional unit to each individual quantity value and validates its reference and dimension.

## Requirements

- Units attach to individual quantity values through the optional `unit_id` field in each quantity payload; type identity does not determine a unit.
- A present Unit ID resolves at the value construction/persistence boundary and matches the fixed quantity dimensions: Angle→Angle, Duration→Time, Span→Length, Percentage→Dimensionless, and Ratio→Dimensionless. Range uses one unit for both bounds and derives its dimension from that Unit. Mismatches and unresolved references fail with `InvalidArgument` before writes; malformed persisted references fail with `CorruptData` on read.
- Unit metadata survives construction, serialization, persistence, and decoding.
- Existing unitless values remain readable according to the selected compatibility rule.

## Dependencies

- ER-0033 Caliper Units and Quantities.

## Acceptance Criteria

- Tests cover each of the six quantity types, the listed type-to-dimension mapping, Range bound consistency, dimension mismatch, missing and wrong-type unit references, serialization round trip, and existing v1 values with absent `unit_id`. Unitless values remain readable; a present `unit_id` is always validated.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Decision

Units attach to individual values. Existing unitless values remain valid; a present `unit_id` must resolve and match the quantity's dimension.

### Live Pass (Human)

- Required: No
- Reason: Automated schema/value and persistence tests cover unit attachment.

## Completion Record

- Implementation: `CaliperValueRegistry` persists value-level unit references and quantity components. Fixed quantity dimensions validate canonical dimension names and components. Unitless legacy records remain readable; malformed or mismatched references fail with the specified error codes.
- Local validation: `make -j2`; `make -C tests test_refract_bootstrap CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'`; `DYLD_LIBRARY_PATH="$PWD/src/.libs" ./tests/test_refract_bootstrap` (14 checks, 0 failures); `DYLD_LIBRARY_PATH="$PWD/src/.libs" TMPDIR=/private/tmp make -j1 check CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` (30/30); `git diff --check`.
- Independent review: initial finding that fixed dimension names could spoof their meaning was resolved by checking canonical components; follow-up review found no remaining issue.
- Delivery state: locally complete; AR-0017 PR/CI and final batch verification pending.
