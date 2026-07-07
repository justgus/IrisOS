---
Legacy-ID: ER-0015
GitHub-Issue: #96
Source-Path: docs/ER/ER-0015-Phase5-Comms-Primitives.md
---

# T-0103 — Phase 5 Milestone 1: Comms Primitives v0

## Task Metadata

- Task ID: T-0103
- Legacy ID: ER-0015
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #96
- AR Dependencies: -
- Date Requested: 2026-02-25
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0015-Phase5-Comms-Primitives.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #96
---

# ER-0015 — Phase 5 Milestone 1: Comms Primitives v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0015
- Title: Phase 5 Milestone 1 — Comms Primitives v0
- Status: Verified
- Date: 2026-02-25
- Owners: Mike
- Type: Enhancement

## Context

Phase 5 begins with minimal Comms primitives to enable ByteStream-style communication.

## Goals

- Implement Comms::Channel, ByteStream, DatagramPort.
- Provide in-memory loopback for testing.

## Non-Goals

- Full TCP/IP stack or device drivers.

## Scope

- In scope:
  - Basic send/recv API.
- Out of scope:
  - Real network integration.

## Requirements

- Functional:
  - ByteStream send/recv works with waitables.

## Proposed Approach

- Implement in-memory loopback ByteStream.

## Acceptance Criteria

- ByteStream round-trip works in tests.

## Risks / Open Questions

- API design for future transports.

## Dependencies

- ER-0007 Phase 2 Integration

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Send/recv demo.
```
