---
GitHub-Issue: #357
AR-Dependencies: AR-0023
ER-Dependencies: ER-0055, ER-0079
---

# ER-0103 — Refract Capability Metadata Validation

## Roles

- Implementation Engineer: implements and validates batch scope.
- System Engineer: decides the broader extended policy profile.

## ER Metadata

- ER ID: ER-0103
- Title: Refract Capability Metadata Validation
- Status: Verified
- Date: 2026-10-15
- Owners: Mike
- Type: Enhancement

## Context

Independent AR-0023 review found that the existing operation capabilities decoder accepts malformed
non-array containers. In particular null becomes an empty requirements list. The existing encoder
writes an array of strings when capabilities are present. This ER repairs that defined format; it
neither defines a broader policy model nor declares the entire extended profile complete.

## Goals

- Reject malformed persisted capability containers rather than silently weakening requirements.
- Prove existing operation requirements survive file-backed reopen and inherited listing.

## Non-Goals

- Deciding whether required_capabilities alone completes AR-0023 extended policy metadata.
- New grants, policy semantics, authorization architecture, or changes to valid stored records.

## Scope

- In scope: existing capabilities array/string wire shape, corruption errors, and regression coverage.
- Out of scope: changing absent/empty capability semantics, grant naming rules, or runtime enforcement.

## Requirements

- If capabilities is present, its value must be an array of strings.
- Missing capabilities and an empty array remain valid and mean no requirements.
- Malformed null, scalar, object, and mixed-element containers return CorruptData on decode.
- Multiple requirements retain their values and order after persistence/reopen, direct retrieval,
  and inherited operation listing.

## Proposed Approach

Validate the existing capabilities shape at operation decoding, following existing typed corruption
handling. Add tests against raw stored CBOR and fresh registry instances after file-backed reopen.
This bounded repair is authorized by the selected AR-0023 batch and does not depend on the pending
architectural scope ruling. It preserves the established encoded format and all valid semantics.

## Acceptance Criteria

1. Raw stored null, object, scalar, and non-string capability entries are rejected with CorruptData.
2. Missing capability metadata, empty arrays, and valid string arrays continue to decode.
3. Two requirements survive file-backed close/reopen and direct plus inherited operation listing.
4. Existing Refract bootstrap, dispatch, and Conch integration tests pass unchanged.

## Risks / Open Questions

- Malformed historic records will now fail explicitly; no migration is needed for records produced
  by the existing encoder. No claim is made that malformed records were previously authoritative.
- AR-0023 policy-scope ruling remains pending independently of this format repair.

## Dependencies

- Verified ER-0055 and ER-0079; existing SchemaRegistry and OperationRegistry paths.

## Implementation Notes

- Status Approved by the explicitly requested batch delegation for existing-format validation.
- Independent review required for persisted metadata and the failure boundary.

## Verification Plan

- `make -j4`
- `env TMPDIR=/private/tmp make -C tests check TESTS='test_refract_capabilities test_refract_registry test_refract_bootstrap test_conch_authoring'`
- `env TMPDIR=/private/tmp make check`
- Independent review must inspect raw malformed-record tests and rerun the focused suite.
- `./bootstrap.sh` and `./configure` regenerate local build rules for the new dedicated test target;
  tracked generated outputs must not be published.

### Independent Live Pass

- Required: No.
- Reason: this is a deterministic persisted-data decoder and local API round-trip; all criteria
  can be faithfully validated by independent executable tests without a physical or observed UI gate.
- Result: Not Required.

### Implementation Evidence

- Before the decoder repair, the new three-check test program reproduced acceptance of null
  capabilities: two compatibility/persistence checks passed, corruption check failed on case 0.
- Sandbox invocation failed before assertions with Check communication-file creation error;
  the escalated baseline reproduced the actual defect.
- Independent planning review confirmed the scope and requirement/test mapping without changing
  the pending AR-0023 policy decision. A dedicated test source avoids growing the large registry suite.
- `./bootstrap.sh` and `./configure` passed to generate local rules for the dedicated test target.
  The configure sandbox emitted a sysctl permission warning but completed successfully.

### Completed Validation

- `make -j4`: passed.
- Focused Refract/Conch suite: passed 4/4 programs; first sandbox attempt failed before assertions
  with Check communication-file error, escalated run passed.
- `env TMPDIR=/private/tmp make check`: passed 32/32 programs.
- Independent [review](../Plans/AR-Batches/reviews/ER-0103-independent-review-2026-10-15.md): all ER
  criteria Pass, fresh focused run 4/4 programs and 55 checks; no findings. The reviewer did not
  repeat the parent full suite.
- Status Verified on 2026-10-15 after implementation commit
  `a3ee08ac40ebc8e34547c936e8544d704723ce31` passed Linux CI
  [38759330650](https://github.com/justgus/IrisOS/actions/runs/38759330650): bootstrap, configure,
  build, unit tests, and distcheck all succeeded. Independent review and local validation above
  complete the remaining gates; Independent Live Pass is Not Required. No broader AR-0023
  completion or policy-scope decision is claimed.
