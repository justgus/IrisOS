---
GitHub-Issue: N/A
---

# ER-0058 — Capability Context Objects and Persistence

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0058
- Title: Capability Context Objects and Persistence
- Status: Complete
- Date: 2026-05-26
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Service Plane Stage D requires capability-aware service context, but the current service substrate has no persisted capability context object that can be attached to service, task, or object identity.
- Background / constraints: AR-0005 defines explicit capabilities and access policies as part of service isolation. AR-0022 stages this work after persistent service lifecycle and registry behavior.

## Goals

- Introduce a first-class capability context model.
- Persist capability context records in Referee.
- Support deterministic reload and lookup by context ID and attached subject ID.

## Non-Goals

- Full policy enforcement.
- Service-boundary authorization checks.
- Sandbox isolation semantics.
- Conch command surfaces for capability administration.

## Scope

- In scope: capability context identity, subject attachment, optional sandbox attachment, and explicit grant names.
- In scope: Referee persistence and reload helpers.
- In scope: tests for round-trip persistence, subject lookup, and invalid grant rejection.
- Out of scope: policy engine, capability service implementation, and runtime enforcement at service boundaries.

## Requirements

- Functional: capability contexts can be persisted and reloaded through Referee.
- Functional: capability contexts can be listed by attached subject object ID.
- Functional: empty or duplicate capability grant names are rejected deterministically.
- Non-functional: persisted grant ordering is deterministic.

## Proposed Approach

- Summary: add a small `CapabilityContext` data model and `CapabilityContextStore` persistence helper under the service subsystem, encoded as Referee object payloads.
- Alternatives considered: folding context fields directly into service descriptors was rejected because task, service, and future sandbox subjects need a reusable context object.

## Acceptance Criteria

- Tests verify capability context persistence survives store reopen.
- Tests verify lookup by subject returns the attached contexts.
- Tests verify empty and duplicate grant names are rejected.

## Risks / Open Questions

- Risk: this creates the data layer before policy enforcement, so callers may assume grants are enforced earlier than they are.
- Question: which grant taxonomy should become canonical before ER-0060 service-boundary checks?

## Dependencies

- Dependency 1: AR-0005 Service Plane Model.
- Dependency 2: AR-0022 Staged Service Plane Delivery.
- Dependency 3: ER-0057 Service Host Lifecycle and Persistent Registry.

## Implementation Notes

- Notes for implementer: keep this ER limited to persisted context data and lookup behavior. Service-boundary enforcement belongs to a follow-on ER.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect persisted capability context records by context ID and subject ID.
