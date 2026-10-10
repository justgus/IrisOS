---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0032
---

# ER-0096 — Astra Float and Double Format Contract

## ER Metadata

- ER ID: ER-0096
- Title: Astra Float and Double Format Contract
- Status: Complete
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 specifies Float as IEEE-754 binary32 and Double as binary64. Current Astra definitions are empty marker types, and tests only verify their presence. The batch selects the existing persisted `TypeDefinition::kind` field to record the format explicitly, so schema inspection and persistence expose the width contract.

## Requirements

- Astra::Float definitions persist `kind=ieee754-binary32` and Astra::Double definitions persist `kind=ieee754-binary64`.
- Reopening the store and decoding both definitions returns the same format metadata.
- Bootstrap migrates existing metadata-less v1 Float/Double definitions to v2 definitions that supersede them; it preserves the old definition records and is idempotent.
- Existing non-floating numeric types retain their current behavior.

## Dependencies

- ER-0032 Astra Math and Structured Types.

## Acceptance Criteria

- The batch decision to use persisted `TypeDefinition::kind` metadata is recorded in the batch ledger.
- Tests prove Float is IEEE-754 binary32 and Double IEEE-754 binary64 after bootstrap, store reopen, and definition decoding.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Decision

The batch uses explicit persisted format metadata in the existing `TypeDefinition::kind` field. This keeps the nominal Astra type IDs while making the required representation visible to schema inspection and stable across persistence.

The v2 definitions migrate pre-existing metadata-less v1 records without rewriting them.

## Implementation Notes

- `kind` is already serialized and decoded as part of `TypeDefinition`; no generic schema migration or new field is required.
- New stores register Float/Double format definitions as v2. Normal bootstrap on an existing store appends a v2 definition that supersedes each metadata-less v1 definition; old definitions remain readable and unchanged.
- This ER describes the schema's format contract. It does not add a numeric value codec, which is outside the existing Astra bootstrap behavior.

### Live Pass (Human)

- Required: No
- Reason: Automated schema and persistence tests verify format identity.
