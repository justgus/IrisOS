---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0032
---

# ER-0096 — Astra Float and Double Format Contract

## ER Metadata

- ER ID: ER-0096
- Title: Astra Float and Double Format Contract
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 specifies Float as IEEE-754 binary32 and Double as binary64. Current Astra definitions are empty marker types, and tests only verify their presence. This ER makes the required format verifiable without presupposing whether nominal type identity alone or explicit schema metadata is the accepted mechanism.

## Requirements

- The selected representation makes binary32/binary64 format and width observable and stable through type inspection and persistence.
- Numeric serialization and conversion preserve the declared format; invalid or unsupported format metadata is rejected deterministically.
- Existing non-floating numeric types retain their current behavior.

## Dependencies

- ER-0032 Astra Math and Structured Types.

## Acceptance Criteria

- The System Engineer's choice between nominal primitive identity with fixed width and explicit format/bit-width metadata is recorded before implementation approval.
- Tests prove Float is IEEE-754 binary32 and Double IEEE-754 binary64 after bootstrap, persistence round trip, and decoding.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Open Question

Should IEEE-754 format and width be guaranteed by nominal Float/Double identity, or represented explicitly in persisted schema metadata?

### Live Pass (Human)

- Required: No
- Reason: Automated schema and persistence tests verify format identity.
