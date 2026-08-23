# T-0183 — Deterministic Machine Lease Lifecycle

## Task Metadata

- Task ID: T-0183
- Status: Active
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

- `make check`
