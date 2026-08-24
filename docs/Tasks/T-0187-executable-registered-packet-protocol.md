# T-0187 — Executable Registered-Packet Protocol

## Task Metadata

- Task ID: T-0187
- Status: Implemented - Not Verified
- Epic: EP-002
- Parent Task: T-0170
- Sprint Assigned: SP-006
- Estimate: 8 points
- Priority: High

## Goal

Demonstrate the EP-002 stack by executing one bounded framing protocol over a Machine-backed loopback session.

## Acceptance Criteria

1. The example encodes a registered Machine packet, sends it through a compatible session, decodes it, and verifies equality.
2. Malformed framing and incompatible transports fail deterministically.
3. The example requires no network access or hardware driver.
4. Existing Comms behavior remains stable.

## Approved Framing Boundary

Use one bounded frame containing a 32-bit unsigned big-endian payload length followed by exactly
that many payload bytes. Reject declared lengths above the implementation's explicit maximum,
truncated payloads, and trailing bytes deterministically. This framing demonstrates execution
without defining a broader protocol taxonomy.

The explicit maximum payload is 1 MiB (1,048,576 bytes). The executable example requires an open
session referencing a compatible stream transport and uses an existing `Channel::loopback()` pair.
It processes exactly one frame and performs no network or hardware access.

## Validation

- `make check`

Result: 29 test programs passed, including 12 Comms transport, session, compatibility, framing, and
execution checks.

## Implementation Notes

- Execution rejects `Machine::Packet` unless `machine::kPacketType` is present in the supplied
  `Refract::SchemaRegistry`, as approved by the System Engineer.
- Frames use one 4-byte unsigned big-endian length and enforce the 1 MiB payload maximum.
- Exact decoding rejects truncated headers, truncated payloads, trailing bytes, and oversized
  declarations.
- Execution requires an open session referencing the supplied compatible stream transport and uses
  only `Channel::loopback()` for the round trip.
