# T-0183 — Deterministic Machine Lease Lifecycle

## Task Metadata

- Task ID: T-0183
- Status: Backlog
- Epic: EP-002
- Parent Task: T-0168
- Sprint Assigned: SP-004
- Estimate: 5-8 points
- Priority: High

## Goal

Add deterministic ownership and lifecycle semantics to Machine handles without introducing clock policy.

## Acceptance Criteria

1. Leases record owner, handle, and capability context.
2. Supported states are `Active`, `Released`, and `Revoked`.
3. Release and revocation invalidate subsequent authorized use deterministically.
4. Wall-clock expiration and background cleanup are absent.
5. Tests cover lifecycle transitions, ownership mismatch, and invalid use.

## Validation

- `make check`
