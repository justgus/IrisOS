---
GitHub-Issue: N/A
ER-Dependencies: ER-0057
---

# AR-0022 — Staged Service Plane Delivery

- Status: Proposed
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

- Draft ER-0057 to implement Stage B and the first part of Stage C.
- Follow with capability-context ERs for Stage D.
