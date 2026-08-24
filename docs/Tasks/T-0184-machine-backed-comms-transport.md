# T-0184 — Machine-Backed Comms Transport

## Task Metadata

- Task ID: T-0184
- Status: Active
- Epic: EP-002
- Parent Task: T-0169
- Sprint Assigned: SP-005
- Estimate: 5 points
- Priority: High

## Goal

Represent a Comms transport over a Machine descriptor or authorized handle while preserving the existing loopback data path.

## Acceptance Criteria

1. A transport records stable identity, stream or datagram semantics, and its Machine resource reference.
2. Handle-backed construction rejects invalid leases.
3. Existing loopback behavior remains available and unchanged.
4. Tests exercise descriptor-backed and authorized handle-backed transports.

## Validation

- `make check`

## Planning Notes

- The transport may reference a Machine descriptor or an SP-004 authorized handle and lease.
- The first implementation remains in-memory and preserves the existing loopback data path.
- Real hardware access, drivers, and remote networking are out of scope.
