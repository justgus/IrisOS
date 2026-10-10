---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0031
---

# ER-0093 — Crate Set Index Contract

## ER Metadata

- ER ID: ER-0093
- Title: Crate Set Index Contract
- Status: Complete
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0017 describes Set as unordered while listing `index` among the standard collection operations. The current Set schema omits `index`; this ER adds and tests the required Refract operation declaration. The repository does not provide a runtime Crate collection implementation, so the schema contract does not define positional behavior or iteration order.

## Requirements

- `Crate::Set` exposes the standard schema `index` operation with a U64 index parameter and Bytes value result. The signature matches Array/List/Tuple because Set elements are values; it declares operation shape only and does not promise a stable positional lookup.
- This ER validates the Refract schema contract only; it does not introduce runtime Set indexing or imply an iteration order.

## Dependencies

- ER-0031 Crate Collections.

## Acceptance Criteria

- Tests assert the operation's name, scope, parameter type, and output type after bootstrap and store reopen.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Decision

`Crate::Set` declares the same `index(U64) -> Bytes` operation signature as Array/List/Tuple,
following those existing schema declarations. For Set the operation is an introspection contract
only; it does not impose iteration order or promise positional lookup. The repository has no
runtime Crate collection implementation in this scope.

### Live Pass (Human)

- Required: No
- Reason: Automated schema and repeatability tests cover the selected contract.
