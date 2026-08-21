# T-0180 — Registered Memory Topology Model

## Task Metadata

- Task ID: T-0180
- Status: Active
- Epic: EP-002
- Parent Task: T-0166
- Sprint Assigned: SP-003
- Estimate: 5 points
- Priority: High

## Goal

Define and register address-space, memory-region, and available-memory-block types.

## Approved Scope

- Address spaces carry stable resource IDs, address widths, and ordered regions.
- Memory regions carry start address, size, alignment, and kind.
- Regions may overlap to represent firmware and memory-mapped views.
- Available blocks must be contained within one region and must not overlap one another.
- Availability is descriptive and grants no allocation or access authority.

## Acceptance Criteria

1. Address spaces and memory blocks use registered Machine scalar types.
2. Overlap, containment, size, and alignment behavior are deterministic.
3. Definitions do not imply mapped or accessible memory.
4. Tests cover valid and invalid topology relationships and registration.

## Validation

- `make check`
