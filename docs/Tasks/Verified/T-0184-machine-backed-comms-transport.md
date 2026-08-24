# T-0184 — Machine-Backed Comms Transport

## Task Metadata

- Task ID: T-0184
- Status: Verified
- Date Verified: 2026-08-24
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

- `autoreconf -fi`
- `./configure CXXFLAGS="-g -O2 -Wno-unused-const-variable -Wno-unused-private-field"`
- `make -j4`
- `make check`

Result: 29 tests passed, including 6 Comms transport and session checks. Validation ran in an
isolated `/private/tmp` copy so Autotools regeneration did not alter generated files in the working
tree.

## Planning Notes

- The transport may reference a Machine descriptor or an SP-004 authorized handle and lease.
- The first implementation remains in-memory and preserves the existing loopback data path.
- Real hardware access, drivers, and remote networking are out of scope.

## Implementation Notes

- Added metadata-only transports with stable identity and explicit stream or datagram semantics.
- Descriptor-backed construction validates available-memory, device, and memory-mapped IO resource
  categories against `MachineInventory`.
- Lease-backed construction validates the lease ID, owner, and capability context against the
  current `MachineLeaseRegistry` state.
- Existing `Channel` and `DatagramPort` implementations are unchanged.

## Verification

Accepted by the System Engineer on 2026-08-24.
