# T-0187 — Executable Registered-Packet Protocol

## Task Metadata

- Task ID: T-0187
- Status: Backlog
- Epic: EP-002
- Parent Task: T-0170
- Sprint Assigned: SP-005
- Estimate: 5-8 points
- Priority: High

## Goal

Demonstrate the EP-002 stack by executing one bounded framing protocol over a Machine-backed loopback session.

## Acceptance Criteria

1. The example encodes a registered Machine packet, sends it through a compatible session, decodes it, and verifies equality.
2. Malformed framing and incompatible transports fail deterministically.
3. The example requires no network access or hardware driver.
4. Existing Comms behavior remains stable.

## Planning Gate

Select the concrete framing protocol when SP-005 is repokered; do not assume a protocol taxonomy before that review.

## Validation

- `make check`
