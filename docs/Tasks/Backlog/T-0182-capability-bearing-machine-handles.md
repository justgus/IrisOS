# T-0182 — Capability-Bearing Machine Handles

## Task Metadata

- Task ID: T-0182
- Status: Backlog
- Epic: EP-002
- Parent Task: T-0168
- Sprint Assigned: SP-004
- Estimate: 5 points
- Priority: High

## Goal

Create descriptor-backed memory, device, and IO-region handles whose construction validates capability context.

## Acceptance Criteria

1. Handles reference known descriptors without duplicating resource facts.
2. Missing or insufficient capability context is rejected deterministically.
3. A handle conveys no real hardware-access implementation.
4. Tests cover authorized and unauthorized construction.

## Validation

- `make check`
