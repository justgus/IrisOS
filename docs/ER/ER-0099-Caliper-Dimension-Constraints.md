---
GitHub-Issue: N/A
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075, ER-0076
---

# ER-0099 — Caliper Dimension Constraint Contract

## ER Metadata

- ER ID: ER-0099
- Title: Caliper Dimension Constraint Contract
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0019 requires constraints such as absolute zero across Kelvin, Celsius, and Fahrenheit, plus higher-level context-dependent constraints. Existing planned catalog and conversion ERs do not define where or how constraints are represented. This ER isolates the v1 constraint scope and validation behavior.

## Requirements

- The selected scope states whether v1 supports intrinsic dimension/unit constraints only, or also general context-dependent constraints.
- Absolute-zero validation is consistent across Kelvin, Celsius, and Fahrenheit after conversion to a shared physical basis.
- Constraint definitions are deterministic, versioned, and do not alter conversion compatibility or catalog identity.
- Context-dependent checks, if selected, identify the required context and fail explicitly when it is absent or invalid.

## Dependencies

- ER-0075 Full Caliper Catalog Expansion.
- ER-0076 Runtime Conversion and Compatibility Engine.

## Acceptance Criteria

- The System Engineer's constraint-scope ruling and its AR acceptance mapping are recorded before implementation approval.
- Tests cover values below, at, and above absolute zero expressed in K, °C, and °F, including exact boundary behavior; contextual cases are included only if that model is selected.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Open Question

Should v1 model intrinsic constraints such as absolute zero only, or also general context-dependent constraints?

### Live Pass (Human)

- Required: No
- Reason: Automated conversion and constraint boundary tests cover the selected scope.
