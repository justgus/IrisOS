---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0033
---

# ER-0098 — Quantity Unit Attachment Contract

## ER Metadata

- ER ID: ER-0098
- Title: Quantity Unit Attachment Contract
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 requires units and dimension metadata for quantities. Current quantity schemas expose optional `unit_id`, while ordinary values have no generic unit field. This ER defines whether unit identity belongs on quantity types, values, or both, and tests the selected persisted representation.

## Requirements

- The selected model states whether units attach to quantity types, individual values, or both, and defines precedence if both are supported.
- Unit identity resolves to a Caliper unit and agrees with the quantity's dimension; mismatches and unresolved references fail deterministically.
- Unit metadata survives construction, serialization, persistence, and decoding.
- Existing unitless values remain readable according to the selected compatibility rule.

## Dependencies

- ER-0033 Caliper Units and Quantities.

## Acceptance Criteria

- The System Engineer's attachment-model ruling is recorded before implementation approval.
- Tests cover unitful and unitless quantities, dimension mismatch, missing unit references, serialization round trip, and compatibility with existing records.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Open Question

Should units attach to quantity types, individual values, or both?

### Live Pass (Human)

- Required: No
- Reason: Automated schema/value and persistence tests cover unit attachment.
