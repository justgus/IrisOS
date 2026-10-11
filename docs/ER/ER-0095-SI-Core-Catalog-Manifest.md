---
GitHub-Issue: N/A
AR-Dependencies: AR-0019
ER-Dependencies: ER-0033
---

# ER-0095 — SI Core Catalog Manifest

## ER Metadata

- ER ID: ER-0095
- Title: SI Core Catalog Manifest
- Status: Verified
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

The existing catalog contains 67 unit records but omits seven of the 22 named SI derived units and uses `ohm` and `um` instead of the canonical symbols `Ω` and `µm`. This ER defines the fixed SI core manifest independently of the customary-unit inventory and prefix representation. Unit names retain existing project spellings; factors, dimensions, and symbols follow the BIPM SI reference. The System Engineer approved publishing the expanded manifest as immutable catalog v2 under a new stable ID, preserving the existing v1 catalog and all of its references unchanged.

## Requirements

- The catalog has the seven missing named derived units: steradian, henry, lumen, becquerel, gray, sievert, and katal, with corresponding dimensions and exact conversion factors.
- Existing SI base and named derived units are compared against an explicit 29-unit manifest: seven base plus 22 named derived units.
- SI symbol assertions use `Ω` and `µm`; unit names retain existing project spellings.
- The expanded manifest is persisted in immutable `CALIPER-BASE-002` with `catalog_version: 2`; `CALIPER-BASE-001` and every record it references remain unchanged and readable.
- This ER does not choose a customary category boundary or whether prefixes are metadata or unit records.

## Dependencies

- ER-0033 Caliper Units and Quantities.
- ER-0075 Full Caliper Catalog Expansion depends on this SI core manifest.

## Acceptance Criteria

- The deterministic manifest test verifies all 29 identities, project names, dimensions, canonical symbols, and exact factors against the selected source.
- Existing non-SI entries remain unaffected.
- Fresh bootstrap creates v2; upgrade from a store containing v1 preserves the v1 catalog and all referenced object IDs, versions, definition references, and payload bytes while adding v2.
- The catalog loader selects v2 by its stable object ID and does not mix legacy v1 unit spellings into the active manifest.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Catalog Versioning Decision

- Ruling (2026-10-10): immutable catalog v2 is approved. It uses a new stable object ID, `CALIPER-BASE-002`; the existing `CALIPER-BASE-001` remains immutable and readable as legacy v1.
- Ruling (2026-10-10): retain project unit-name spellings and use canonical SI symbols, including `Ω` and `µm`.

## Implementation Record

- `bootstrap_core_catalog` now writes the 29-unit SI manifest as immutable catalog v2 at `CALIPER-BASE-002`, preserving existing `CALIPER-BASE-001` and historical Unit/Dimension objects. Added the seven missing named SI derived units and canonical `Ω`/`µm` records.
- `load_caliper_catalog` selects the exact v2 catalog identity and reads its serialized dimensions and units, so legacy `ohm`/`um` objects do not collide with the active manifest.
- Automated coverage asserts all 29 names, symbols, dimensions, normalized factors and offsets, selected derived-dimension component maps, repeat bootstrap, store reopen, v1/v2 coexistence, and preservation of legacy catalog, Unit, and Dimension object refs, definition refs, and payload bytes.
- Independent read-only review found no blocking findings after the loader and migration coverage updates.
- Validation passed on macOS `aarch64-apple-darwin27.2.0`: `./bootstrap.sh`; `./configure`; `make -j2 CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'`; `make -C tests check TESTS='test_refract_bootstrap' CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'` (1/1); full `make check` (30/30); `git diff --check`. The first full-suite run hit the stale `test_referee_core` install-name issue; after rebuilding that test against the worktree library, all 30 tests passed. Autotools-generated tracked helpers and the generated test binary were restored.
- This ER is Verified after PR #347 merged and CI run 38166633486 passed.

### Independent Live Pass

- Required: No
- Reason: Automated catalog manifest and factor tests cover the fixed data set.


## Final Batch Verification

- Verified after local validation, independent review where required, PR merge, and PR #347; CI run 38166633486 passing its required CI checks. The detailed evidence is recorded in `docs/Plans/AR-Batch-Progress.md`.
