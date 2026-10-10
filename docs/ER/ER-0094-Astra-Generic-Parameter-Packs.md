---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0031, ER-0032
---

# ER-0094 — Astra Generic Parameter Pack Contract

## ER Metadata

- ER ID: ER-0094
- Title: Astra Generic Parameter Pack Contract
- Status: Proposed
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

`Crate::Tuple` uses the parameter label `Ts`, and `Astra::Tensor` uses `Dims...`, but generic type definitions currently do not encode whether these labels denote variadic parameter packs. Generic instances can carry Type, Value, and Variadic argument forms, but the definition-to-argument contract is missing. This ER defines and validates that mapping after the System Engineer selects a representation.

## Requirements

- The selected design states whether current parameter labels plus `GenericArg` Type/Value/Variadic forms are sufficient, or generic definitions need persisted parameter-kind/role metadata.
- Tuple accepts the selected number and kinds of type arguments; Tensor dimension arguments conform to the selected pack and value/type rules.
- Invalid arity, wrong argument kind, and malformed pack encodings fail deterministically at construction and persistence boundaries.
- Existing non-pack generic definitions preserve their current validation behavior.

## Dependencies

- ER-0031 Crate Collections.
- ER-0032 Astra Math and Structured Types.

## Acceptance Criteria

- Parent AR and tests state the selected persisted/API representation before implementation approval.
- Tests cover zero, one, and multiple pack members, malformed/empty cases where disallowed, round trip through persistence, and ordinary fixed-arity generic compatibility.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Open Question

Keep current labels and express concrete arguments through existing Type/Value/Variadic forms, or add persisted parameter-kind/role metadata to generic definitions?

### Live Pass (Human)

- Required: No
- Reason: Automated API and persistence tests cover generic argument validation.
