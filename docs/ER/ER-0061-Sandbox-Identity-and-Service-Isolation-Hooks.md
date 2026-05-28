---
GitHub-Issue: #268
---

# ER-0061 — Sandbox Identity and Service Isolation Hooks

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0061
- Title: Sandbox Identity and Service Isolation Hooks
- Status: Verified
- Date: 2026-05-26
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: ER-0058 allowed capability contexts to reference an optional sandbox object ID, and ER-0060 added service-boundary capability enforcement, but sandbox identity is still only an opaque ID with no persisted metadata or IPC hook.
- Background / constraints: AR-0022 Stage D calls for sandbox identifiers and service boundary checks. The System Engineer selected a metadata/hooks-only ER-0061 slice, with sandbox enforcement deferred.

## Goals

- Introduce first-class persisted sandbox identity metadata.
- Allow sandbox identity to be attached to service IPC envelopes.
- Preserve sandbox identity through simple request/response flows.
- Keep ER-0060 capability enforcement unchanged.

## Non-Goals

- Mandatory sandbox identity checks at service boundaries.
- Sandbox membership enforcement.
- Process, filesystem, memory, network, or OS-level isolation.
- Full policy engine behavior.

## Scope

- In scope: sandbox identity ID, display name, and attached subject object IDs.
- In scope: deterministic Referee persistence and reload for sandbox identity records.
- In scope: subject lookup for sandbox identity records.
- In scope: optional sandbox identity metadata on service message envelopes.
- Out of scope: rejecting service requests based on sandbox identity.

## Requirements

- Functional: sandbox identity records can be persisted and reloaded through Referee.
- Functional: sandbox identity records can be listed by attached subject object ID.
- Functional: empty sandbox names and duplicate subject attachments are rejected deterministically.
- Functional: service responses preserve request sandbox identity metadata.
- Non-functional: existing service authorization behavior remains grant-only unless later ERs add sandbox enforcement.

## Proposed Approach

- Summary: extend the capability context store with sandbox identity records and add an optional sandbox ID field to service IPC envelopes.
- Alternatives considered: making sandbox identity mandatory in ER-0060 authorization was rejected for this slice because service isolation semantics are not defined yet.

## Acceptance Criteria

- Tests verify sandbox identity persistence survives store reopen.
- Tests verify lookup by subject returns attached sandbox identity records.
- Tests verify invalid sandbox metadata is rejected.
- Tests verify service IPC response envelopes preserve sandbox identity metadata.

## Risks / Open Questions

- Risk: callers may mistake sandbox identity metadata for isolation enforcement.
- Question: which ER should define mandatory sandbox membership checks at service boundaries?

## Dependencies

- Dependency 1: ER-0058 Capability Context Objects and Persistence.
- Dependency 2: ER-0060 Service Boundary Capability Enforcement.
- Dependency 3: AR-0022 Staged Service Plane Delivery.

## Implementation Notes

- Notes for implementer: keep this ER metadata/hooks only.
- Notes for implementer: do not change ER-0060 grant enforcement semantics.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - persist a sandbox identity, reload it, attach its ID to an IPC request, and inspect the response envelope.
