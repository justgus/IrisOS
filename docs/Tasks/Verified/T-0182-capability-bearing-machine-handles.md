# T-0182 — Capability-Bearing Machine Handles

## Task Metadata

- Task ID: T-0182
- Status: Verified
- Date Verified: 2026-08-24
- Epic: EP-002
- Parent Task: T-0168
- Sprint Assigned: SP-004
- Estimate: 5 points
- Priority: High
- Owner: Implementation Engineer

## Goal

Create descriptor-backed memory, device, and IO-region handles whose construction validates capability context.

## Approved Scope

- Handles reference immutable SP-003 descriptors by stable resource ID.
- Handle construction requires a capability context that authorizes the requested resource and access mode.
- Memory, device, and IO-region handle categories remain distinct.
- Handles carry authority metadata only; they perform no hardware access.

## Out of Scope

- Hardware drivers, device operations, memory mapping, and direct IO.
- Descriptor mutation or persistence.
- Lease expiration and background cleanup.

## Acceptance Criteria

1. Handles reference known descriptors without duplicating resource facts.
2. Missing or insufficient capability context is rejected deterministically.
3. A handle conveys no real hardware-access implementation.
4. Tests cover authorized and unauthorized construction.

## Validation

- `autoreconf -fi`
- `./configure CXXFLAGS="-g -O2 -Wno-unused-const-variable -Wno-unused-private-field"`
- `make -j4`
- `make check`

Result: 28 tests passed, including 6 Machine authority checks. Validation ran in an isolated
`/private/tmp` copy so Autotools regeneration did not alter generated files in the working tree.

## Implementation Notes

- Added typed memory, device, and IO-region handles with `Read`, `Write`, and `Control` modes.
- Capability names use `machine.<resource-kind>.<access-mode>:<resource-id>`.
- Handle acquisition loads the capability context from `CapabilityContextStore`; caller-created
  context values are not trusted.
- Memory handles target available-memory blocks, and IO-region handles target only memory-mapped
  regions.
- Handles contain no hardware operations.

## Verification

Accepted by the System Engineer on 2026-08-24.
