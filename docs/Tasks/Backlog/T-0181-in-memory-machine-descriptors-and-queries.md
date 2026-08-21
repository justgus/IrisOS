# T-0181 — In-Memory Machine Descriptors and Queries

## Task Metadata

- Task ID: T-0181
- Status: Backlog
- Epic: EP-002
- Parent Task: T-0167
- Sprint Assigned: SP-003
- Estimate: 5 points
- Priority: High

## Goal

Represent and query deterministic runtime facts for processors, memory, buses, and devices without persistence or authority.

## Acceptance Criteria

1. Descriptors represent concrete processor cores, register files, memory regions, buses, and devices.
2. Deterministic identifiers and ordering support in-memory lookup and enumeration.
3. Descriptors expose no resource-access operations.
4. Tests exercise representative inventory construction and queries.

## Validation

- `make check`
