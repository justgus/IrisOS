# T-0180 — Registered Memory Topology Model

## Task Metadata

- Task ID: T-0180
- Status: Backlog
- Epic: EP-002
- Parent Task: T-0166
- Sprint Assigned: SP-003
- Estimate: 5 points
- Priority: High

## Goal

Define and register address-space, memory-region, and available-memory-block types.

## Acceptance Criteria

1. Address spaces and memory blocks use registered Machine scalar types.
2. Overlap, containment, size, and alignment behavior are deterministic.
3. Definitions do not imply mapped or accessible memory.
4. Tests cover valid and invalid topology relationships and registration.

## Validation

- `make check`
