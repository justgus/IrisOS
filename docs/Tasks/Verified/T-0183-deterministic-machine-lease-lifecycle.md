# T-0183 — Deterministic Machine Lease Lifecycle

## Task Metadata

- Task ID: T-0183
- Status: Verified
- Date Verified: 2026-08-24
- Epic: EP-002
- Parent Task: T-0168
- Sprint Assigned: SP-004
- Estimate: 8 points
- Priority: High
- Owner: Implementation Engineer

## Goal

Add deterministic ownership and lifecycle semantics to Machine handles without introducing clock policy.

## Approved Scope

- A lease associates one owner and capability context with one Machine handle.
- Lease state is explicit and limited to `Active`, `Released`, and `Revoked`.
- Release and revocation are terminal and deterministically prevent subsequent authorized use.
- Ownership mismatch and invalid state transitions fail without mutating the lease.

## Out of Scope

- Wall-clock expiration, renewal, background cleanup, and concurrent arbitration.
- Hardware access or resource scheduling.
- Persistence of handles or leases.

## Acceptance Criteria

1. Leases record owner, handle, and capability context.
2. Supported states are `Active`, `Released`, and `Revoked`.
3. Release and revocation invalidate subsequent authorized use deterministically.
4. Wall-clock expiration and background cleanup are absent.
5. Tests cover lifecycle transitions, ownership mismatch, and invalid use.

## Validation

- `autoreconf -fi`
- `./configure CXXFLAGS="-g -O2 -Wno-unused-const-variable -Wno-unused-private-field"`
- `make -j4`
- `make check`

Result: 28 tests passed, including 6 Machine authority checks. Validation ran in an isolated
`/private/tmp` copy so Autotools regeneration did not alter generated files in the working tree.

## Implementation Notes

- Added in-memory leases with explicit `Active`, `Released`, and `Revoked` states.
- Lease creation loads the persisted capability context and requires the owner to match its subject.
- Owner release and supervisor revocation are terminal; invalid transitions leave state unchanged.
- Supervisors can revoke one lease or all active leases for a stored owner or context ID without
  requiring the originating objects to remain available.
- Automatic revocation on object disappearance, clocks, persistence, and background cleanup remain
  out of scope.

## Verification

Accepted by the System Engineer on 2026-08-24.
