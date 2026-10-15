---
GitHub-Issue: #290
ER-Dependencies: ER-0057, ER-0058, ER-0059, ER-0060, ER-0061, ER-0062
---

# AR-0022 — Staged Service Plane Delivery

- Status: In Progress
- Date: 2026-03-14
- Owners: Mike

## Context

AR-0005 correctly identifies the need for a Service Plane, but its current language collapses several
different maturity levels into a single accepted target. The codebase currently implements a local
service contract, an in-memory registry, and synchronous request/response dispatch. It does not yet
implement persistent service registry state, deterministic service host lifecycle management, or
capability-aware service isolation.

This gap makes it hard to plan follow-on ERs cleanly because "the Service Plane" currently refers to
both the implemented local substrate and a much broader future runtime.

## Recommendation

Amend the Service Plane architecture to define explicit delivery stages.

### Stage A: Local Service Contract

- Service descriptor and service object contract
- Standardized message envelope
- Local in-node registry and request/response dispatch
- Deterministic in-process test coverage

### Stage B: Service Host Lifecycle

- Task-backed service hosts
- Deterministic create, start, stop, and restart transitions
- Health and restart semantics
- Service host state inspection

### Stage C: Persistent Service Registry

- Service descriptors persisted in Referee
- Registry rebuild and recovery behavior
- Stable discovery across process or shell restarts

### Stage D: Isolation And Policy

- Capability-aware service context
- Sandbox identifiers and service boundary checks
- Minimal policy enforcement hooks

### Stage E: Expanded Core Services

- Network-oriented services
- Memory allocation service
- Sandbox or capability service as a first-class service
- Additional system services as accepted later

## Goals

- Preserve AR-0005 as the architectural source of truth while making delivery stages explicit.
- Allow ERs to target one maturity level at a time.
- Avoid implying that all Service Plane properties are already part of the same verified surface.

## Non-Goals (v1)

- Redefining the Service Plane away from the service-object model
- Locking in a remote or distributed transport model
- Defining every future core service in implementation detail

## Proposed Model

- AR-0005 remains the umbrella architecture recommendation.
- This follow-on AR defines the staged realization model for AR-0005.
- Verification should be claimed against one stage at a time.
- Later ERs should state which stage they advance.

## Relationship To Existing ARs

- Refines [AR-0005-Service-Plane-Model.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0005-Service-Plane-Model.md)
- Aligns with [AR-0006-CEO-Exec-Runtime-Model.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0006-CEO-Exec-Runtime-Model.md)

## Next Steps

- ER-0057 verification was confirmed by the System Engineer on 2026-10-14.
- Define later Stage E services through separate accepted implementation work.

## Implementation Progress

- ER-0057 now provides Stage B host lifecycle and Stage C persisted descriptor discovery/recovery.
  The System Engineer selected explicit binding and start for recovered hosts. Its local build
  and tests pass; the System Engineer confirmed its Verified status and prior live verification
  on 2026-10-14.
- Verified ER-0058 through ER-0061 provide capability context persistence, task attachment,
  boundary enforcement, and sandbox identity/isolation hooks for Stage D.
- Verified ER-0062 provides a metadata-only memory service baseline for Stage E. Allocation,
  network services, and a first-class sandbox/capability service are broader follow-on scope.
- This AR remains In Progress because its staged recommendation includes unfinished Stage E scope.
