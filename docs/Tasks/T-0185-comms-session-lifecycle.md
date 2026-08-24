# T-0185 — Comms Session Lifecycle

## Task Metadata

- Task ID: T-0185
- Status: Active
- Epic: EP-002
- Parent Task: T-0169
- Sprint Assigned: SP-005
- Estimate: 5 points
- Priority: High

## Goal

Add deterministic endpoint, transport, and lifecycle metadata for Comms sessions.

## Acceptance Criteria

1. A session references two endpoints and one transport.
2. Supported states are `Created`, `Open`, `Closing`, and `Closed`.
3. Invalid state transitions fail deterministically.
4. Tests exercise lifecycle transitions over the loopback transport.

## Validation

- `make check`

## Planning Notes

- Sessions are deterministic metadata and lifecycle objects above an SP-005 transport.
- Lifecycle transitions perform no network access or background work.
- Protocol compatibility and framing remain in SP-006.
