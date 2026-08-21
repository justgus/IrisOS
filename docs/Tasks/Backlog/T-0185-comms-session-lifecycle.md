# T-0185 — Comms Session Lifecycle

## Task Metadata

- Task ID: T-0185
- Status: Backlog
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
