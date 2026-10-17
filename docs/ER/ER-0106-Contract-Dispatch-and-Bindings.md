---
GitHub-Issue: N/A
AR-Dependencies: AR-0027
ER-Dependencies: ER-0105
---

# ER-0106 — Contract Dispatch and Bindings

## Roles

- Implementation Engineer: drafts, implements and validates.
- System Engineer: approves architecture and verifies completed ERs.

## ER Metadata

- ER ID: ER-0106
- Title: Contract Dispatch and Bindings
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

1. Resolve exact contract identity/version using every ordered input type; reject inconsistent counts; preserve deterministic subtype/optional/depth ranking and fail ambiguous matches.
2. Distinguish generic instances with different arguments; reject mismatched outputs and absent conformances; same-name different contracts cannot collide.
3. Persist binding identity/version/route; rebind local callbacks explicitly after recovery; unbound/delegated unsupported execution fails with typed errors.
4. Check caller capabilities independently of effects; unauthorized calls never reach callbacks, and callback/route failures propagate through the existing error model.
5. Validate output shape/types and return result values separately; failures and successful calls do not assign to an unrelated caller destination.

## Risks / Open Questions

The System Engineer approved the plan's architectural choices on 2026-10-17.
Persisted metadata must reject corruption instead of silently dropping constraints.

## Dependencies

- ER-0105.

## Implementation Notes

Dedicated test target: test_contract_dispatch.

## Verification Plan

- `./bootstrap.sh` and `./configure` when adding Automake sources or test targets.
- `make -j4`
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_contract_dispatch test_refract_registry test_refract_bootstrap test_conch_authoring'`
- `env TMPDIR=/private/tmp make check`
- Record actual completed results and inspect the final diff before publication.

### Independent Live Pass

- Required: No.
- Reason: deterministic local storage, metadata, invocation and shell behavior can be tested.
- Result: Not Required for approved local scope; no remote/hardware or human observation claimed.

### Implementation evidence — 2026-10-17

- Added ContractDispatcher typed resolution and process-local callback registry with explicit
  receiver/capability/route checks and result count/type validation. Callback failures stay typed.
- `./bootstrap.sh`, `./configure`, `make -j4`: passed for new sources/test target.
- Focused suite (all three new contract targets plus existing registry/bootstrap/Conch) passed
  6/6 programs; 4 new dispatch checks cover every input, ambiguity, optional/subtype/inherited
  resolution, registered generic identity mismatches, authority/callback/result errors and reopen.
- Sandbox execution failed before assertions on Check communication temporary-file permissions;
  escalated rerun passed. No independent review or human observation claimed.
- Complete; System Engineer verification remains outstanding.
