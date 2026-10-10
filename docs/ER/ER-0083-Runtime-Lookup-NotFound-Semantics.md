---
GitHub-Issue: N/A
AR-Dependencies: AR-0001, AR-0011
ER-Dependencies: ER-0081, ER-0082
---

# ER-0083 — Runtime Lookup NotFound Semantics

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass.

## ER Metadata

- ER ID: ER-0083
- Title: Runtime Lookup NotFound Semantics
- Status: Verified
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: service, capability, task, and artifact-object lookups still expose absence as an optional value or as an untyped error string, despite AR-0001's typed `NotFound` contract.
- Background / constraints: these APIs sit above Referee and Refract. This ER applies only the accepted cross-cutting error contract and does not add or change service, capability, task, or Vizier behavior.

## Goals

- Use `ErrorCode::NotFound` for absent named service, memory region, capability record, task, or requested artifact object.
- Update consumers to propagate or handle `NotFound` explicitly.

## Non-Goals

- Change service lifecycle, task scheduling, capability policy, or transport behavior.
- Treat normal no-data or no-route outcomes as errors.

## Scope

- In scope: `ServiceRegistry::resolve_by_name` and `resolve_by_type`, `MemoryService::lookup_region`, `CapabilityContextStore::get_sandbox` and `get_context`, `TaskRegistry::get_task`, and missing-artifact handling in Vizier's `spawn_concho_for_artifact`.
- Out of scope: nonblocking `IoExecutor::recv_datagram` returning no packet, Vizier route functions returning no route, and private service-resolution helpers.

## Requirements

- Functional: missing named service, type service, memory region, capability record, or task returns `ErrorCode::NotFound`.
- Functional: invalid input remains distinguishable from not-found; existing typed errors remain intact.
- Functional: requesting a Concho for a missing artifact returns `ErrorCode::NotFound`, while a valid artifact with no route remains a successful empty optional result.
- Non-functional: present records and ordinary no-data/no-route behavior remain unchanged.

## Proposed Approach

- Summary: migrate only public entity lookup APIs to direct `Result<T>` results or typed missing-object errors, update their call sites and focused tests, and leave absence that represents a normal nonblocking or routing outcome optional.
- Alternatives considered: converting every optional result was rejected because an empty datagram receive or an unroutable relationship is a valid outcome, not a missing object.

## Acceptance Criteria

- Tests prove each in-scope absent entity produces `ErrorCode::NotFound` and present entities still resolve.
- Tests prove invalid input is not misclassified as `NotFound`.
- Tests prove no-datagram and no-route results remain successful empty optionals.
- Existing service, capability, CEO task, and Vizier routing tests pass.

## Risks / Open Questions

- Risk: consumers may treat an absent service/task as an expected branch and need explicit NotFound handling.
- Question: none; the distinction between missing entities and normal no-data/no-route outcomes is specified above.

## Dependencies

- Dependency 1: ER-0081 Referee Object Lookup NotFound Semantics.
- Dependency 2: ER-0082 Refract Lookup NotFound Semantics.
- Dependency 3: AR-0001 Expected-Style Error Model.

## Implementation Notes

- Do not change no-data semantics for nonblocking receives or no-route semantics for Vizier.
- Keep typed errors across service and capability boundaries.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_service_ipc test_capability_context test_ceo_tasks test_vizier_routing`
  - `make check`
- Manual checks: none; automated tests cover the in-scope API and its callers.

### Live Pass (Human)

- Required: No
- If no, reason: Unit and integration tests cover API errors; no visible or target-device behavior changes.
- Build / environment / target: N/A
- Steps: N/A
- Expected observations: N/A
- Observed results: N/A
- Result: Not Required
- Performed by: N/A
- Date: N/A
