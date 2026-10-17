---
GitHub-Issue: #355
AR-Dependencies: AR-0022, AR-0024
ER-Dependencies: ER-0057, ER-0060, ER-0073, ER-0074
---

# ER-0101 — Local Comms Network Service

## Roles

- Implementation Engineer: implements and validates batch scope.
- System Engineer: rules on architecture and records required human observations.

## ER Metadata

- ER ID: ER-0101
- Title: Local Comms Network Service
- Status: Verified
- Date: 2026-10-14
- Owners: Mike
- Type: Enhancement

## Context

AR-0022 Stage E calls for network-oriented services; AR-0024 provides the Machine-backed Comms
substrate. This proposed first service uses that local substrate without selecting a remote stack.

## Goals

- Expose session lifecycle and framed packet round trips through a first-class service.
- Retain Machine lease authorization, protocol compatibility, payload bounds, and subject ownership.

## Non-Goals

- Cross-host transport, socket protocols, dynamic negotiation, drivers, and a complete network stack.

## Scope

- In scope: trusted constructor bindings for existing Comms protocol/transport/lease registry and
  schema registry; bounded session collection; open/round-trip/close endpoints.
- Out of scope: caller-created hardware authority and descriptor-only permission bypasses.

## Requirements

- Requests must belong to the authorized transport owner and revalidate its Machine lease.
- Sessions carry stable IDs, endpoint/transport identities, and strict open/close state.
- Unknown sessions, wrong subjects, closed sessions, invalid selectors/CBOR, excessive payloads,
  and incompatible/revoked Machine mappings fail deterministically.

## Proposed Approach

Wrap existing Comms Session and execute_registered_packet_round_trip. Borrow only trusted binding
registries whose lifetimes exceed the binding; copy protocol/transport and own session state in the
handler. The System Engineer approved this scope; prerequisite ER-0073/0074 are Verified.

## Acceptance Criteria

1. IPC opens a session over the authorized Machine-backed transport and returns its identity.
2. IPC round-trip preserves payload, response correlation, and sandbox; inspectable session metadata
   records endpoints, transport, and state.
3. Close rejects subsequent traffic and illegal duplicate transitions; session count is bounded.
4. Cross-subject use, revoked leases, incompatible mappings, and descriptor-only transport cannot
   execute packets or obtain session authority.
5. Malformed, oversized, conflicting-selector, omitted-selector, and unknown-session requests fail
   without partially publishing sessions or packets.
6. Existing Comms primitive and session tests remain passing.

## Risks / Open Questions

- Approved scope: a local Comms-backed service is the initial Stage E network slice.
- No remote authentication or delivery guarantees beyond the existing local substrate are claimed.

## Dependencies

- Verified ER-0057/0060/0073/0074.

## Implementation Notes

- Independent review required for Machine authority, borrowed lifetimes, and session behavior.

## Verification Plan

- `make -j4`
- `TMPDIR=/private/tmp make -C tests check TESTS='test_network_service test_core_services test_comms_primitives test_comms_transport_session test_machine_authority test_service_host'`
- `TMPDIR=/private/tmp make check`
- Independent review must repeat focused tests and attempt lease, owner, and closed-session bypasses.

### Independent Live Pass

- Required: No for the proposed deterministic local contract.
- Reason: local Machine descriptors/leases, sessions, and packet round trips are faithfully testable
  without real hardware, external network, or a user-observed surface.
- Result: Not Required; bounded scope approved by the System Engineer on 2026-10-15.

### System Engineer Ruling — 2026-10-15

- The System Engineer approved the drafted AR-0022 Stage E scope, including this ER.
  Earlier statements that the architectural ruling is pending are superseded.
- Public API/security/lifetime changes require separate independent review under the batch workflow.

### Implementation and Validation — 2026-10-15

- NetworkService copies trusted protocol/transport bindings, borrows documented registry inputs,
  owns bounded sessions, and checks owner/lease/compatibility for every request. It uses the
  existing registered framed loopback packet path. Closed records remain bounded and inspectable.
- `./bootstrap.sh`, `./configure`, `make -j4`: passed for new source and dedicated test target.
  Generated tracked outputs excluded. Initial test fixture bus ID and binary comparison compiler
  errors were corrected before validation; no production compiler failure remains.
- Focused command in plan: passed 6/6 programs, 39 independent checks.
- `env TMPDIR=/private/tmp make check`: passed 34/34. Check required outside-sandbox execution
  after communication-file failures before assertions.
- [Independent review](../Plans/AR-Batches/reviews/ER-0101-independent-review-2026-10-15.md):
  every criterion Pass, no blocking findings. Independent Live Pass: Not Required.
- Complete; exact implementation CI must pass before Verified.

### Linux CI Repair — 2026-10-15

- Run [38813348896](https://github.com/justgus/IrisOS/actions/runs/38813348896) for cc54489
  failed `make check V=1`: 33/34 passed; network lifecycle case returned Timeout at the success
  helper. Build passed; distcheck skipped. Downloaded test_network_service.log confirms the error.
- Diagnosis: the 1 MiB frame/loopback/CBOR round trip exceeded the test's 10 ms IPC deadline on
  Linux CI. No acceptance criterion imposes that deadline. The test now allows 5 seconds while
  keeping the exact payload limit, equality, authorization, and lifecycle assertions intact.
- Focused and full revalidation plus new-head CI required; ER remains Complete.

- Repair validation: focused network suite passed 1/1; full suite passed 35/35 after normal
  relinking of the restored generated core launcher. Production code unchanged; all existing
  independent criterion evidence retained. New implementation-head CI remains required.

### CI Verification — 2026-10-17

- Corrected combined implementation head `6e5677ae522e4b22de42931fae7e0604d6f503cc`
  passed [Linux CI run 38831614893](https://github.com/justgus/IrisOS/actions/runs/38831614893),
  completed 2026-10-16: bootstrap, configure, build, unit tests, and distcheck all succeeded.
- Prior independent criterion review has no open findings; corrected local full suite passed
  35/35. Independent Live Pass: Not Required under the approved bounded scope.
- Complete -> Verified on completed evidence. This reconciliation changes documentation only;
  no fresh local build/test run or human observation is claimed.
