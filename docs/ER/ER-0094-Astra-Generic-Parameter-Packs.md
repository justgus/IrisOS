---
GitHub-Issue: N/A
AR-Dependencies: AR-0017
ER-Dependencies: ER-0031, ER-0032
---

# ER-0094 — Astra Generic Parameter Pack Contract

## ER Metadata

- ER ID: ER-0094
- Title: Astra Generic Parameter Pack Contract
- Status: Complete
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

`Crate::Tuple` uses the parameter label `Ts`, and `Astra::Tensor` uses `Dims...`, but generic type definitions currently do not encode whether these labels denote variadic parameter packs. Generic instances can carry Type, Value, and Variadic argument forms, but the definition-to-argument contract is missing. This ER adds an in-code validator keyed by the stable Tuple and Tensor base TypeIDs; it does not infer pack behavior from arbitrary user parameter labels or add persisted parameter metadata. Other generic definitions retain existing validation behavior.

## Requirements

- Keep the existing labels and encode packs with a `GenericArgKind::Variadic` wrapper; each Tuple item is a Type argument and each Tensor extent is a positive U64 Value argument.
- Tuple permits an empty pack. Tensor requires at least one extent after its element type.
- `GenericRegistry::register_instance` validates Tuple/Tensor arguments before persistence; `get_instance_by_type` and `ScopedTypeRegistry::resolve_or_register` apply the same validation when loading or resolving. Tuple takes exactly one outer Variadic argument containing zero or more Type arguments. Tensor takes one Type argument followed by exactly one Variadic argument containing one or more positive U64 Value arguments. Malformed or nested packs, wrong kinds, invalid U64 values, and extra/missing outer arguments fail with `InvalidArgument` before writes; malformed stored instances fail with `CorruptData` on read.
- Existing non-pack generic definitions preserve their current validation behavior.

## Dependencies

- ER-0031 Crate Collections.
- ER-0032 Astra Math and Structured Types.

## Acceptance Criteria

- Tests exercise the registry construction and read/resolve paths for zero, one, and multiple Tuple pack members; one and multiple Tensor extents; wrong item kinds, nested/malformed packs, missing/extra outer args, malformed stored records, and fixed-arity compatibility. Rejected construction leaves no persisted generic instance; old non-pack definitions and instances retain their current decoding behavior.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Decision

Keep `Ts` and `Dims...` labels. Use existing Type/Value/Variadic `GenericArg` forms; do not add persisted parameter-kind metadata.

### Live Pass (Human)

- Required: No
- Reason: Automated API and persistence tests cover generic argument validation.

## Completion Record

- Implementation validates Tuple and Tensor pack shapes before registration and scoped resolution, and validates matching persisted instances on lookup. Pack-specific raw payload checks reject unknown or missing argument-kind metadata without changing decoding for other generic definitions.
- Local validation: `make -j CXXFLAGS='-g -O2 -Wno-unused-const-variable -Wno-unused-private-field'`; focused registry suite passed 1/1; full `make check` passed 30/30; `git diff --check` passed.
- Independent read-only review found and drove fixes for malformed stored argument tags and TypeID-zero registration/read consistency. Final review and fresh focused validation passed with no findings.
