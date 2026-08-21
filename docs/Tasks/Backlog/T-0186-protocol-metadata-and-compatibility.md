# T-0186 — Protocol Metadata and Compatibility

## Task Metadata

- Task ID: T-0186
- Status: Backlog
- Epic: EP-002
- Parent Task: T-0170
- Sprint Assigned: SP-005
- Estimate: 5 points
- Priority: High

## Goal

Define inspectable protocol requirements and deterministic compatibility checks over Machine-backed transports.

## Acceptance Criteria

1. Protocol metadata declares required transport semantics and Machine resource properties.
2. Compatible and incompatible mappings are inspectable and deterministic.
3. Compatibility checks do not perform dynamic negotiation or IO.
4. Tests cover representative accepted and rejected mappings.

## Validation

- `make check`
