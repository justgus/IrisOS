---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0031
---

# ER-0093 — Crate Set Index Contract

## ER Metadata

- ER ID: ER-0093
- Title: Crate Set Index Contract
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 describes Set as unordered while listing `index` among the standard collection operations. Existing `Crate::Set` schema declares `index`, but there is no accepted semantic contract for what indexing an unordered set means. This ER is limited to resolving that contract and adding matching schema/runtime tests. It does not assume that `index` means insertion order, sorted order, or internal storage position.

## Requirements

- The System Engineer's ruling determines whether Set exposes `index`, and if so, the stable semantics and error behavior.
- Tests verify the selected contract through the public collection API/schema and prove that results do not depend on hash-table iteration order or process-specific layout.
- If indexing is excluded, tests assert that Set does not advertise it; the parent AR's acceptance mapping must explain the resolved interpretation.

## Dependencies

- ER-0031 Crate Collections.

## Acceptance Criteria

- The selected Set index contract is documented in the parent AR or this ER before implementation approval.
- Focused tests cover empty, singleton, multiple-element, absent-element, and repeated-run behavior applicable to the selected contract.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Open Question

Should unordered `Crate::Set` expose `index` with a defined stable ordering, or should it omit `index` despite AR-0017's general operation list?

### Live Pass (Human)

- Required: No
- Reason: Automated schema and repeatability tests cover the selected contract.
