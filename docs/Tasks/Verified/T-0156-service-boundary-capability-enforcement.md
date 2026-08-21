---
Legacy-ID: ER-0060
GitHub-Issue: #267
Source-Path: docs/ER/ER-0060-Service-Boundary-Capability-Enforcement.md
---

# T-0156 — Service Boundary Capability Enforcement

## Task Metadata

- Task ID: T-0156
- Legacy ID: ER-0060
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #267
- AR Dependencies: -
- Date Requested: 2026-05-26
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0060-Service-Boundary-Capability-Enforcement.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #267
---

# ER-0060 — Service Boundary Capability Enforcement

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0060
- Title: Service Boundary Capability Enforcement
- Status: Verified
- Date: 2026-05-26
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: ER-0058 added persisted capability contexts and ER-0059 attached capability context identity to tasks, but service IPC still accepts calls without service-boundary capability checks.
- Background / constraints: AR-0022 Stage D calls for capability-aware service context, sandbox identifiers, service boundary checks, and minimal policy enforcement hooks.

## Goals

- Allow services and declared endpoints to expose required capability grants.
- Enforce required grants before dispatching service IPC requests.
- Use persisted capability context records as the initial authority source.
- Preserve existing IPC behavior for services with no required grants.

## Non-Goals

- Full policy engine.
- Sandbox identity and isolation semantics.
- Task-registry-based provenance checks.
- Network or distributed service authorization.

## Scope

- In scope: required grant metadata on service descriptors and endpoint descriptors.
- In scope: service-boundary authorizer abstraction.
- In scope: persisted capability-context authorizer using sender subject lookup.
- In scope: IPC rejection when required grants are missing.
- Out of scope: broader policy taxonomy, sandbox enforcement, and task-attached context validation.

## Requirements

- Functional: a service can declare required grants for boundary access.
- Functional: a declared endpoint can declare additional required grants.
- Functional: IPC rejects requests when the sender has no persisted capability context with the required grants.
- Functional: IPC allows requests when the sender's persisted capability contexts satisfy the required grants.
- Non-functional: services without required grants continue to dispatch as before.

## Proposed Approach

- Summary: add explicit grant metadata to service descriptors and endpoints, introduce a small `ServiceBoundaryAuthorizer` hook, and implement a `CapabilityContextAuthorizer` that checks persisted grants attached to the request sender subject.
- Alternatives considered: hidden service-name grant conventions were rejected because service requirements should be inspectable. Coupling IPC directly to CEO task registry was deferred because current message envelopes carry sender object identity, not task identity.

## Acceptance Criteria

- Tests verify descriptor-level required grants reject missing sender grants.
- Tests verify descriptor-level required grants allow matching persisted sender grants.
- Tests verify endpoint-level required grants reject and allow when a request identifies a declared endpoint.
- Existing IPC tests continue to pass for unrestricted services.

## Risks / Open Questions

- Risk: sender-subject lookup does not prove task provenance; task-aware validation remains future hardening.
- Risk: endpoint metadata enforcement only applies when the request identifies a declared endpoint.
- Question: should ER-0061 make sandbox identity mandatory for service-boundary checks?

## Dependencies

- Dependency 1: ER-0058 Capability Context Objects and Persistence.
- Dependency 2: ER-0059 CEO Task Capability Attachment.
- Dependency 3: AR-0022 Staged Service Plane Delivery.

## Implementation Notes

- Notes for implementer: keep the authorizer small and fail closed only when services declare required grants.
- Notes for implementer: preserve unrestricted IPC behavior for service descriptors with no required grants.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - register a service with required grants, send a request from a sender without a matching persisted capability context, then persist a matching context and retry.
```
