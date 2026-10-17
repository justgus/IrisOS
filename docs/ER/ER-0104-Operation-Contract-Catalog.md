---
GitHub-Issue: N/A
AR-Dependencies: AR-0027
ER-Dependencies: 
---

# ER-0104 — Operation Contract Catalog

## Roles

- Implementation Engineer: drafts, implements and validates.
- System Engineer: approves architecture and verifies completed ERs.

## ER Metadata

- ER ID: ER-0104
- Title: Operation Contract Catalog
- Status: Complete
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

1. Persist stable identity/version, display name, category, ordered typed signature, documentation, effects and required capabilities as canonical Refract catalog metadata.
2. Registration rejects zero/invalid identities or versions, malformed type expressions and duplicate parameter names; identical records are idempotent, conflicting same-version records fail without replacement.
3. Lookup/listing returns deterministic results; distinct identities sharing a name remain distinct; older versions remain available.
4. File-backed reopen preserves exact metadata; malformed persisted records return CorruptData, and closed-store errors propagate.

## Risks / Open Questions

The System Engineer approved the plan's architectural choices on 2026-10-17.
Persisted metadata must reject corruption instead of silently dropping constraints.

## Dependencies

- Existing Refract and Referee.

## Implementation Notes

Dedicated test target: test_operation_catalog.

## Verification Plan

- `./bootstrap.sh` and `./configure` when adding Automake sources or test targets.
- `make -j4`
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_operation_catalog test_refract_registry test_refract_bootstrap test_conch_authoring'`
- `env TMPDIR=/private/tmp make check`
- Record actual completed results and inspect the final diff before publication.

### Independent Live Pass

- Required: No.
- Reason: deterministic local storage, metadata, invocation and shell behavior can be tested.
- Result: Not Required for approved local scope; no remote/hardware or human observation claimed.

### Implementation evidence — 2026-10-17

- Implemented OperationCatalog with immutable ObjectID/version keys, parameterized signatures,
  deterministic discovery filters, documented effects/capabilities and strict CBOR decoding.
- `./bootstrap.sh`, `./configure`, `make -j4`: passed. Regeneration required for new library
  sources/test target; tracked generated libtool and platform test launcher excluded from commits.
- Focused command above passed 4/4 programs, including 3 new catalog checks. First compilation
  exposed a Check runner type mismatch, corrected using normal suite registration. Sandbox run
  failed before assertions on communication-file creation; escalated rerun passed.
- Status Complete, awaiting System Engineer verification. No independent review claimed.
