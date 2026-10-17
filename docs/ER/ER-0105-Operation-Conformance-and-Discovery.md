---
GitHub-Issue: N/A
AR-Dependencies: AR-0027
ER-Dependencies: ER-0104
---

# ER-0105 — Operation Conformance and Discovery

## Roles

- Implementation Engineer: drafts, implements and validates.
- System Engineer: approves architecture and verifies completed ERs.

## ER Metadata

- ER ID: ER-0105
- Title: Operation Conformance and Discovery
- Status: Verified
- Date: 2026-10-17
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Follow existing C++20 conventions; keep the diff bounded and generated output unpublished.

## Context

AR-0027 requires explicit standard contracts extending existing Refract/Conduit mechanisms.
The current inventory and proposed architectural choices are in
[the implementation plan](../Plans/AR-0027-Implementation-Plan.md).

## Goals

Deliver the criteria below using canonical Refract metadata and existing IrisOS errors.

## Non-Goals

Remote transport protocols, vector/matrix kernels, implicit numeric conversions, expression syntax,
and unrelated changes to the AR-0016 name-based dispatch model.

## Scope

- In scope: this ER's acceptance criteria and compatibility tests.
- Out of scope: architectural alternatives outside the approved plan.

## Requirements

Preserve existing stored definitions and independent capability enforcement. Do not infer approval,
conformance, authorization, compatibility or successful verification from a display name.

## Proposed Approach

Use the corresponding work item and architectural proposal in the linked plan. The System Engineer approved the plan and ER scope on 2026-10-17.

## Acceptance Criteria

1. Persist explicit contract references and concrete type substitutions on operation definitions without changing legacy operation decoding.
2. Validate scope, ordered input/output types and optionality after substitution; reject unknown contract versions, missing substitutions and incompatible declarations.
3. Discover contracts by identity/version, category and required capability; list declared/inherited support by type and stored object type with declaring owner and binding details.
4. File-backed reopen preserves references; malformed metadata errors rather than advertising conformance; no name-based implicit conformance or object authority widening.

## Risks / Open Questions

The System Engineer approved the plan's architectural choices on 2026-10-17.
Persisted metadata must reject corruption instead of silently dropping constraints.

## Dependencies

- ER-0104.

## Implementation Notes

Dedicated test target: test_operation_conformance.

## Verification Plan

- `./bootstrap.sh` and `./configure` when adding Automake sources or test targets.
- `make -j4`
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_operation_conformance test_refract_registry test_refract_bootstrap test_conch_authoring'`
- `env TMPDIR=/private/tmp make check`
- Record actual completed results and inspect the final diff before publication.

### Independent Live Pass

- Required: No.
- Reason: deterministic local storage, metadata, invocation and shell behavior can be tested.
- Result: Not Required for approved local scope; no remote/hardware or human observation claimed.

### Implementation evidence — 2026-10-17

- Explicit conformance/substitutions/binding metadata persists alongside existing operations.
  Schema registration validates the entire claim before publishing; discovery revalidates reads.
- `./bootstrap.sh`, `./configure`, `make -j4`: passed for new sources and test target.
- Focused suite including test_operation_catalog in addition to the planned targets passed 5/5
  programs; 3 new conformance checks cover incompatible/unknown claims, inheritance/object views,
  file-backed reopen and malformed stored metadata. Existing legacy metadata suites pass.
- Sandbox execution failed before assertions on Check communication-file permissions; escalated
  rerun passed. No generated output published and no independent review claimed.
- Complete; System Engineer verification remains outstanding.

### System Engineer Acceptance — 2026-10-17

- The System Engineer explicitly instructed marking parent AR-0027 complete, accepting its
  delivered ER-0104–0107 scope. Complete -> Verified by that System Engineer decision.
- Completed implementation/local validation evidence above is retained. Independent Live Pass:
  Not Required for the approved local scope. No new tests, independent review or human observation
  are claimed by this documentation transition.
- Final-head advisory CI 39052974745 is still in_progress for 278ae38 at this check; no pass is
  inferred. Temporary Initial Development Mode permits acceptance while advisory CI is running.
