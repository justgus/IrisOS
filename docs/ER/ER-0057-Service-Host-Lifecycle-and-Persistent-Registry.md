---
GitHub-Issue: #264
---

# ER-0057 — Service Host Lifecycle and Persistent Registry

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0057
- Title: Service Host Lifecycle and Persistent Registry
- Status: Verified
- Date: 2026-03-14
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: the current Service Plane implementation stops at an in-memory registry and synchronous local dispatch, with no durable service records or deterministic host lifecycle model.
- Background / constraints: AR-0005 defines the Service Plane direction, and AR-0022 breaks that direction into delivery stages. This ER targets the first concrete expansion beyond the current local substrate.

## Goals

- Introduce service host records and lifecycle state.
- Persist service descriptors and registry bindings in Referee.
- Support deterministic create, start, stop, and restart behavior for task-backed service hosts.

## Non-Goals

- Full network or distributed service transport.
- Full policy engine or complete sandbox model.
- Every future core service defined in one ER.

## Scope

- In scope: service host metadata and lifecycle states.
- In scope: persistent registry records and recovery behavior.
- In scope: CEO-backed service host execution model and restart semantics.
- In scope: Conch inspection and control surfaces needed to observe lifecycle state.
- Out of scope: network service implementation, memory service implementation, and full capability-service implementation.

## Requirements

- Functional: services can be represented as persistent descriptors and service host records.
- Functional: service hosts can be created, started, stopped, and restarted deterministically.
- Functional: registry state survives process or shell restart through Referee persistence.
- Functional: local service discovery uses persisted registry state when available.
- Non-functional: lifecycle and restart behavior are deterministic and testable.

## Proposed Approach

- Summary: introduce service host and registry objects persisted in Referee, bind service host execution to CEO task lifecycle, and evolve the current service runtime from in-memory-only registration to a persistent service registry with restart semantics.
- Alternatives considered: leaving service lifecycle entirely implicit in process-local code was rejected because it does not support the accepted Service Plane direction.

## Acceptance Criteria

- Persistent registry tests verify service descriptors survive restart and can be rediscovered.
- Lifecycle tests verify create, start, stop, and restart transitions.
- Local service request routing continues to work across service host restarts.

## Risks / Open Questions

- Risk: persistence and in-memory runtime state may diverge if lifecycle transitions are not modeled carefully.
- Question: should restart policy be a field on the service host record in this ER or in a follow-on policy ER?

## Dependencies

- Dependency 1: AR-0022 Staged Service Plane Delivery.
- Dependency 2: AR-0006 CEO/Exec Runtime Model.
- Dependency 3: ER-0001 Service Model and IPC Foundation.
- Dependency 4: ER-0034 Referee Storage Layout Implementation (v1).

## Implementation Notes

- Notes for implementer: keep the first host lifecycle state machine minimal and explicit; do not couple restart semantics to Conch session state.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect service descriptors before and after restart.
  - start, stop, and restart a task-backed service host and verify rediscovery.
