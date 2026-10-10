---
GitHub-Issue: N/A
AR-Dependencies: AR-0019
ER-Dependencies: ER-0033
---

# ER-0095 — SI Core Catalog Manifest

## ER Metadata

- ER ID: ER-0095
- Title: SI Core Catalog Manifest
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

The existing catalog contains 67 unit records but omits seven of the 22 named SI derived units and uses `ohm` and `um` instead of the canonical symbols `Ω` and `µm`. This ER isolates the fixed SI core manifest from the still-open customary-unit inventory and prefix representation choices. Canonical unit names remain subject to ruling #13; factors, dimensions, and symbols follow the selected BIPM SI reference.

## Requirements

- The catalog has the seven missing named derived units: steradian, henry, lumen, becquerel, gray, sievert, and katal, with corresponding dimensions and exact conversion factors.
- Existing SI base and named derived units are compared against an explicit 29-unit manifest: seven base plus 22 named derived units.
- SI symbol assertions use `Ω` and `µm`; the unit-name assertion follows the System Engineer ruling on BIPM spellings versus project spellings.
- This ER does not choose a customary category boundary or whether prefixes are metadata or unit records.

## Dependencies

- ER-0033 Caliper Units and Quantities.
- ER-0075 Full Caliper Catalog Expansion depends on this SI core manifest.

## Acceptance Criteria

- Approval and implementation are gated on ruling #13. The deterministic manifest test verifies all 29 identities, canonical names, dimensions, symbols, and exact factors against the selected source and spelling convention; it must not assert either spelling before that ruling.
- Existing non-SI entries remain unaffected.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Status Gate

- This ER remains Proposed and is not an independent implementation candidate until ruling #13 selects the canonical name convention.

## Open Question

Use canonical BIPM spellings such as `metre` and `litre`, or retain project spellings such as `meter` and `liter`?

### Live Pass (Human)

- Required: No
- Reason: Automated catalog manifest and factor tests cover the fixed data set.
