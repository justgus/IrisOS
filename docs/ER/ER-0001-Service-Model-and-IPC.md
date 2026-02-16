# ER-0001 — Service Model and IPC Foundation

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0001
- Title: Service Model and IPC Foundation
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Epic

## Context

We need a concrete Service Plane model and a baseline inter-object communication (IPC) service
aligned with AR-0005. This ER scopes the minimum work to enable service discovery and message
exchange between objects and services.

## Goals

- Define a Service Object contract in code (types + interfaces).
- Implement a minimal IPC service and service registry for local communication.
- Provide a test harness that validates message send/receive and service discovery.

## Non-Goals

- Full networking stack or remote messaging.
- Persistence of registry state across reboot (phase 1).
- Security policy enforcement beyond baseline capability checks.

## Scope

- In scope:
  - Service Object interface definition (C++ types and headers).
  - IPC service with message envelope, routing, and ACK/timeout behavior.
  - Service registry (register/resolve/unregister).
  - Basic tests for message send/receive and resolve path.
- Out of scope:
  - Distributed messaging and network transport.
  - Advanced scheduling or QoS.

## Requirements

- Functional:
  - Services are addressable by ObjectID and by well-known name/type.
  - IPC supports request/response with correlation ids.
  - Local delivery provides at-least-once semantics within a node.
- Non-functional:
  - Deterministic behavior; no hidden side effects.
  - Minimal external dependencies.
  - C++20 or C++24 baseline.

## Proposed Approach

- Create a `ServiceObject` interface in `src/referee/` or a new `src/services/` module.
- Define `MessageEnvelope` and `Endpoint` types that reuse `ObjectID` and `TypeID`.
- Implement an in-process IPC service (initially) to validate the contract and tests.
- Implement a simple Service Registry service that maps service name/type to ObjectID.

## Acceptance Criteria

- A minimal IPC send/receive test passes with correlation IDs and ACKs.
- Service registry can register and resolve a service by name/type.
- Documentation updated to reflect Service Object model and IPC usage.

## Risks / Open Questions

- Scheduler model and threading semantics are not yet defined.
- Capability/security model for service access is not specified.

## Dependencies

- AR-0005 acceptance.
- Referee ObjectID/TypeID types.

## Implementation Notes

- Keep interfaces minimal and explicit; avoid long files by splitting headers if needed.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - None (phase 1)
