---
GitHub-Issue: #342
AR-Dependencies: AR-0017
ER-Dependencies: ER-0031
---

# ER-0090 — Crate Collection Schema Contract Tests

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may mark this ER Verified only after all documented gates pass.

## ER Metadata

- ER ID: ER-0090
- Title: Crate Collection Schema Contract Tests
- Status: Approved
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small; do not add dependencies.

## Context

- Problem statement: AR-0017 requires a common collection vocabulary and standard collection operations, but the existing bootstrap test checks operation declarations only for `Crate::Array`. Registration of List, Set, Map, and Tuple is asserted, while their type parameters and operation contracts are not.
- Background / constraints: ER-0031 is marked Verified and the bootstrap definitions already declare size/iterate/contains for all five collections, with index declared for Array, List, Map, and Tuple. Set's index applicability is isolated in ER-0093. Tuple declares the parameter label `Ts`, but its variadic semantics are isolated in ER-0094. This ER adds verification coverage only for settled schema declarations and does not change runtime behavior or settle Astra/Caliper open questions.

## Goals

- Assert that the five Crate collection definitions are registered, expose their current parameter labels for schema introspection, and declare the operations whose applicability is settled.
- Defer Set index applicability to ER-0093 and Tuple variadic-parameter semantics to ER-0094.

## Non-Goals

- Add collection runtime storage or operation implementations.
- Change collection type parameters, operation signatures, or AR-0017 semantics.

## Scope

- In scope: tests in the existing Refract bootstrap test program for Array, List, Set, Map, and Tuple definitions and their generic parameter/operation metadata.
- Out of scope: changes to collection schemas unless the new assertions expose a mismatch; any such mismatch requires a separate scoped ER review.

## Requirements

- Functional: bootstrap tests verify all five collection types are registered and expose their current generic parameter labels for schema introspection: `T` on Array/List/Set, `K` and `V` on Map, and `Ts` on Tuple. This ER does not claim that the `Ts` label defines variadic semantics; that contract is covered by ER-0094.
- Functional: tests verify each collection declares the size, iterate, and contains operations named by AR-0017; parameter and return types are not asserted unless an accepted specification defines them.
- Functional: tests verify index declarations for Array, List, Map, and Tuple. Set index applicability is explicitly excluded from this ER and covered by ER-0093 after the System Engineer ruling.
- Non-functional: test failures identify the collection and mismatched operation/parameter clearly.

## Proposed Approach

- Summary: extend `test_bootstrap_crate_collections` with table-driven assertions over the five definitions, checking their AR-defined generic parameters and declared operation names. Keep assertions limited to the accepted contract.
- Alternatives considered: adding runtime collection behavior tests was rejected because current operations are Refract schema declarations and runtime semantics are outside this acceptance gap.

## Acceptance Criteria

- The test fails if any of Array/List/Set/Map/Tuple is absent or its current parameter-label declaration changes unexpectedly.
- The test verifies size, iterate, and contains for all five types, plus index for Array/List/Map/Tuple; it does not elevate bootstrap implementation details into required signatures.
- No assertion in this ER decides Set index applicability or Tuple variadic semantics; those are tracked as ER-0093 and ER-0094 dependencies of AR-0017.
- Existing bootstrap tests and the full test suite pass.

## Risks / Open Questions

- Risk: operation metadata is the only current contract evidence; this ER does not claim runtime implementations exist. AR-0017's “standard operations” wording may need clarification for per-type index semantics; this test ER must not decide it.
- Limitation: this ER verifies declared schema contracts; runtime operation implementations are outside its scope.

## Dependencies

- Dependency 1: ER-0031 Crate Collections.

## Implementation Notes

- Keep the change test-only unless an assertion proves the current schema violates the accepted AR; do not silently modify runtime or schema behavior under this test ER.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_refract_bootstrap'`
  - `make check`
  - `git diff --check`
- Manual checks: Not required; the Refract definition inspection assertions fully observe the declared collection schema contract.

### Live Pass (Human)

- Required: No
- If no, reason: This ER adds schema assertions that are fully observable in automated bootstrap tests; no target-specific environment or interaction is required.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
