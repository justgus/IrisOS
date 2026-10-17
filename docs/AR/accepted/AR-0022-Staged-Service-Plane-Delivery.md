---
GitHub-Issue: #290
ER-Dependencies: ER-0057, ER-0058, ER-0059, ER-0060, ER-0061, ER-0062, ER-0100, ER-0101, ER-0102
---

# AR-0022 — Staged Service Plane Delivery

- Status: Implemented
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
- Approved Stage E services are implemented and verified under ER-0100–0102. Additional
  system services require separately accepted implementation work.

## Implementation Progress

- ER-0057 now provides Stage B host lifecycle and Stage C persisted descriptor discovery/recovery.
  The System Engineer selected explicit binding and start for recovered hosts. Its local build
  and tests pass; the System Engineer confirmed its Verified status and prior live verification
  on 2026-10-14.
- Verified ER-0058 through ER-0061 provide capability context persistence, task attachment,
  boundary enforcement, and sandbox identity/isolation hooks for Stage D.
- Verified ER-0062 provides a metadata-only memory service baseline for Stage E. Allocation,
  network services, and a first-class sandbox/capability service are delivered by Verified
  ER-0100–0102 under the bounded scope approved below.

### Stage E Ruling — 2026-10-15

- The System Engineer approved the bounded owned-allocation, local Comms loopback network, and
  read-only capability/sandbox inspection service scope in ER-0100–0102. This approval authorized
  their implementation; all three ERs are now Verified on the evidence below.

### Stage E Completion — 2026-10-17

- ER-0100 provides bounded owned allocation with owner checks and deterministic release;
  ER-0101 provides authorized local Comms sessions and packet round trips; ER-0102 provides
  read-only caller-filtered capability/sandbox inspection. All nine listed ER dependencies
  are Verified. Existing Stage A–D delivery and verification remain intact.
- Separate independent service reviews passed with no open findings. Corrected local validation
  passed 35/35 programs. [Linux CI run 38831614893](https://github.com/justgus/IrisOS/actions/runs/38831614893)
  passed bootstrap/configure/build/unit tests/distcheck for `6e5677a`, including both final services
  and the test deadline repair. Prior ER-0100 exact-head CI also passed.
- In Progress -> Implemented for the accepted bounded stages. No cross-host networking, raw
  memory addresses, grant mutation, or new OS sandbox enforcement is claimed; additional services
  remain future work. Independent Live Pass is Not Required for the approved Stage E local APIs.
