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
- Status: Approved
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
- Out of scope: unresolved architectural alternatives until the System Engineer approves the plan.

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

The plan's identity, compatibility, conformance, initial catalog and binding choices require approval.
Persisted metadata must reject corruption instead of silently dropping constraints.

## Dependencies

- ER-0106.

## Implementation Notes

No implementation or independent review has occurred. Proposed dedicated test target: test_standard_contracts.

## Verification Plan

- `./bootstrap.sh` and `./configure` when adding Automake sources or test targets.
- `make -j4`
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_standard_contracts test_refract_registry test_refract_bootstrap test_conch_authoring'`
- `env TMPDIR=/private/tmp make check`
- Record actual completed results and inspect the final diff before publication.

### Independent Live Pass

- Required: No (proposed applicability).
- Reason: deterministic local storage, metadata, invocation and shell behavior can be tested.
- Result: Not Required for proposed local scope; no remote/hardware or human observation claimed.
