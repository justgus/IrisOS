# T-0179 — Registered Processor Architecture Model

## Task Metadata

- Task ID: T-0179
- Status: Active
- Epic: EP-002
- Parent Task: T-0166
- Sprint Assigned: SP-003
- Estimate: 8 points
- Priority: High

## Goal

Define and register processor architecture, core, register-definition, and register-file types while preserving the definition-versus-instance boundary.

## Approved Scope

- Architecture definitions carry stable IDs, names, byte order, address width, and ordered core and register definitions.
- Register definitions carry stable IDs, names, bit widths, and roles.
- Core descriptors reference architecture and core definitions and record logical index and enabled state.
- Register-file descriptors identify which register definitions exist for a core.
- Register definitions and files contain no live register values.

## Acceptance Criteria

1. Architecture and register definitions are portable Machine types with stable Refract identities.
2. Concrete cores and register files can reference their definitions deterministically.
3. The model grants no authority and performs no hardware probing.
4. Tests cover representative architecture relationships and registration.

## Validation

- `make check`
