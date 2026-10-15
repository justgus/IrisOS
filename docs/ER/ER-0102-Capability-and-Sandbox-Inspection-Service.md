---
GitHub-Issue: #356
AR-Dependencies: AR-0022
ER-Dependencies: ER-0057, ER-0058, ER-0060, ER-0061
---

# ER-0102 — Capability and Sandbox Inspection Service

## Roles

- Implementation Engineer: implements and validates batch scope.
- System Engineer: rules on architecture and records required human observations.

## ER Metadata

- ER ID: ER-0102
- Title: Capability and Sandbox Inspection Service
- Status: Approved
- Date: 2026-10-14
- Owners: Mike
- Type: Enhancement

## Context

AR-0022 Stage E requires a first-class sandbox/capability service. Existing persisted contexts and
sandbox identity hooks provide a substrate but are not themselves a ServiceObject.

## Goals

- Provide read-only IPC queries for the caller's persisted capability contexts and sandbox identities.
- Preserve the existing metadata/hooks-only sandbox architecture and trusted management APIs.

## Non-Goals

- Issuing/revoking grants, administrative cross-subject queries, mandatory sandbox enforcement,
  process isolation, and redefining existing service authorization.

## Scope

- In scope: a first-class ServiceObject backed by CapabilityContextStore, caller-filtered queries,
  strict selector validation, typed failures, host descriptor persistence, and explicit binding.
- Out of scope: capability mutations and arbitrary subject lookup.

## Requirements

- Ownership is derived only from request.sender, following the existing trusted local envelope
  contract; requests cannot supply a different subject to widen visibility.
- Read persisted records on each request and propagate corruption/storage errors.
- Expose only the caller's sandbox membership, not unrelated subjects attached to that sandbox.

## Proposed Approach

Add a separate read-only service wrapping list_contexts_for_subject/list_sandboxes_for_subject.
Validate matching endpoint/message selectors and reject subject override payloads. The System
Engineer approved this scope on 2026-10-15.

## Acceptance Criteria

1. IPC queries return only request.sender's capability contexts and sandbox memberships.
2. Cross-subject payload overrides and contradictory endpoint selectors fail without information
   disclosure; malformed payloads return typed errors.
3. Responses preserve correlation/sandbox and have deterministic ordering matching the store APIs.
4. File-backed close/reopen preserves query results after host recovery, explicit bind, and start.
5. Corrupt records and failed storage return errors, not empty success.
6. No service endpoint can create, alter, or remove grants or sandbox metadata.

## Risks / Open Questions

- Approved scope: read-only inspection is the first Stage E capability service slice.
- Sender identity is trusted local IPC context, not a new authentication boundary.

## Dependencies

- Verified ER-0057/0058/0060/0061.

## Implementation Notes

- Independent review required for privacy, persisted data decoding, and borrowed store lifetime.

## Verification Plan

- `make -j4`
- `TMPDIR=/private/tmp make -C tests check TESTS='test_core_services test_capability_context test_service_ipc test_service_host'`
- `TMPDIR=/private/tmp make check`
- Independent review must repeat focused validation and attempt cross-subject inspection.

### Independent Live Pass

- Required: No for the proposed read-only local API.
- Reason: filtering, persistence, typed errors, and host lifecycle are faithfully testable locally.
- Result: Not Required; bounded scope approved by the System Engineer on 2026-10-15.

### System Engineer Ruling — 2026-10-15

- The System Engineer approved the drafted AR-0022 Stage E scope, including this ER.
  Earlier statements that the architectural ruling is pending are superseded.
- Public API/security/lifetime changes require separate independent review under the batch workflow.
