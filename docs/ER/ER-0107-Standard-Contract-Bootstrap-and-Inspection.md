---
GitHub-Issue: N/A
AR-Dependencies: AR-0027
ER-Dependencies: ER-0106
---

# ER-0107 — Standard Contract Bootstrap and Inspection

## Roles

- Implementation Engineer: drafts, implements and validates.
- System Engineer: approves architecture and verifies completed ERs.

## ER Metadata

- ER ID: ER-0107
- Title: Standard Contract Bootstrap and Inspection
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

1. Bootstrap documented fixed-identity versioned to_string, compare and add contracts with categories and explicit primitive conformances; repeat bootstrap is idempotent.
2. Bind and invoke primitive implementations; two-input U64/F64 addition produces a separate typed result with documented numeric failure behavior.
3. Conch inspection renders catalog identity/version/signature/category/effects/capabilities/conformance/route from canonical metadata, with deterministic regression-tested output.
4. Existing name-based calls, core operation output, persisted schemas and capability enforcement remain compatible; no vector kernels or remote transport are claimed.

## Risks / Open Questions

The System Engineer approved the plan's architectural choices on 2026-10-17.
Persisted metadata must reject corruption instead of silently dropping constraints.

## Dependencies

- ER-0106.

## Implementation Notes

Dedicated test target: test_standard_contracts.

## Verification Plan

- `./bootstrap.sh` and `./configure` when adding Automake sources or test targets.
- `make -j4`
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_standard_contracts test_refract_registry test_refract_bootstrap test_conch_authoring'`
- `env TMPDIR=/private/tmp make check`
- Record actual completed results and inspect the final diff before publication.

### Independent Live Pass

- Required: No.
- Reason: deterministic local storage, metadata, invocation and shell behavior can be tested.
- Result: Not Required for approved local scope; no remote/hardware or human observation claimed.

### Implementation evidence — 2026-10-17

- Bootstraps three immutable version-1 contracts and adopts primitive to_string/compare plus
  U64/F64 add in new schema versions. Existing definition records and name-based call paths remain.
- Local standard callbacks validate primitive CBOR, reject arithmetic overflow/non-finite values,
  preserve Bytes array display and return results separately from stored destinations.
- Conch exposes `ops --catalog [--category name] [--capability name]` and
  `ops <TypeName> --contracts`, using Refract metadata for signatures, authority and binding routes.
- First focused integration run failed standard adoption because existing Bytes definitions repeat
  core declarations. Bounded repair adopts only the first compatible unbound declaration and
  preserves the duplicate legacy declarations. Conflicting explicit claims still fail.
- Review tightened only conforming-operation decoding for unknown scope, malformed signature arrays,
  and signed/invalid type IDs. Added stored-corruption regression cases; legacy decoding unchanged.
- `./bootstrap.sh`, `./configure`, `make -j4`: passed. New sources/test target required regeneration;
  generated tracked libtool and tests/test_referee_core remain local, excluded from commits.
- Focused seven-program suite passed after the Bytes repair. Final
  `env TMPDIR=/private/tmp make check` passed 39/39 programs after all source/test changes,
  including 4 standard-contract checks, large-U64 ordering and byte-array compatibility.
- Sandbox full run stopped all programs before assertions on Check communication-file creation;
  authorized outside-sandbox rerun passed. No assertion failure is attributed to the sandbox.
- Header installation check passed using `make -C src install-nobase_includeHEADERS
  DESTDIR="$PWD/.ar0027-headercheck"`, followed by a C++20 syntax-only translation unit including
  refract/standard_contracts.h and refract/contract_inspection.h from the staged include directory.
  bootstrap.h and the existing SqliteStore header are installed to satisfy public dependencies.
- `git diff --check` passed. No independent review or human observation claimed. Complete;
  System Engineer verification remains outstanding.

### System Engineer Acceptance — 2026-10-17

- The System Engineer explicitly instructed marking parent AR-0027 complete, accepting its
  delivered ER-0104–0107 scope. Complete -> Verified by that System Engineer decision.
- Completed implementation/local validation evidence above is retained. Independent Live Pass:
  Not Required for the approved local scope. No new tests, independent review or human observation
  are claimed by this documentation transition.
- Final-head advisory CI 39052974745 is still in_progress for 278ae38 at this check; no pass is
  inferred. Temporary Initial Development Mode permits acceptance while advisory CI is running.
