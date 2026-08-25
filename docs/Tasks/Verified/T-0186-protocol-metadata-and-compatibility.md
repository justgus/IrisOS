# T-0186 — Protocol Metadata and Compatibility

## Task Metadata

- Task ID: T-0186
- Status: Verified
- Date Verified: 2026-08-25
- Epic: EP-002
- Parent Task: T-0170
- Sprint Assigned: SP-006
- Estimate: 5 points
- Priority: High

## Goal

Define inspectable protocol requirements and deterministic compatibility checks over Machine-backed transports.

## Acceptance Criteria

1. Protocol metadata declares required transport semantics and Machine resource properties.
2. Compatible and incompatible mappings are inspectable and deterministic.
3. Compatibility checks do not perform dynamic negotiation or IO.
4. Tests cover representative accepted and rejected mappings.

## Validation

- `make check`

Result: 29 test programs passed, including 12 Comms transport, session, compatibility, framing, and
execution checks.

## Approved Scope

- Protocol metadata records stable identity, required stream or datagram semantics, and a nonempty
  set of allowed `MachineResourceKind` values.
- Compatibility results expose accepted/rejected state and a deterministic reason.
- Descriptor-backed transports use their recorded resource kind.
- Lease-backed transports must remain active and matching; their resource kind is resolved from the
  stored lease handle.
- Compatibility performs no negotiation or IO.

## Implementation Notes

- Protocol metadata records stable identity, a nonempty name, required transport semantics, and a
  normalized nonempty set of allowed Machine resource kinds.
- Compatibility reports an explicit reason and resolved resource kind where applicable.
- Authorized transports revalidate lease, owner, and capability-context bindings on every check.
- Compatibility performs no transport IO or negotiation.

## Verification

Accepted by the System Engineer on 2026-08-25.
