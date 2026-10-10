---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0032, ER-0094
---

# ER-0097 — Astra Vector, Matrix, and Tensor Shape Contract

## ER Metadata

- ER ID: ER-0097
- Title: Astra Vector, Matrix, and Tensor Shape Contract
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 requires vectors, matrices, and tensors to carry element type and shape/extents. Current definitions contain generic labels but no separately represented runtime shape. This ER defines the shape contract after the System Engineer selects fixed type parameters or dynamic extents on values. Tensor pack encoding is governed by ER-0094.

## Requirements

- The selected design defines where extents live, whether they are fixed or dynamic, and the validation boundary for rank and extent values.
- Vector, Matrix, and Tensor preserve element type and shape through construction, inspection, and persistence.
- Invalid rank, negative/zero extents where prohibited, and mismatched element counts fail deterministically according to the selected contract.
- No hidden default converts between fixed and dynamic shapes.

## Dependencies

- ER-0032 Astra Math and Structured Types.
- ER-0094 Astra Generic Parameter Pack Contract.

## Acceptance Criteria

- Parent AR and tests record the selected fixed-versus-dynamic semantics before implementation approval.
- Tests cover rank, boundary extents, element-count consistency, and persistence round trips for Vector, Matrix, and Tensor.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Open Question

Should shape extents be fixed type parameters, or dynamic runtime values attached to each vector/matrix/tensor value?

### Live Pass (Human)

- Required: No
- Reason: Automated schema/value and persistence tests cover shape behavior.
