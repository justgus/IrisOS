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
- Status: Complete
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
- Resolved by the System Engineer on 2026-10-13: recover descriptors with stopped, unbound hosts;
  binding and start are explicit. Automatic restart policy is deferred.

## Dependencies

- Dependency 1: AR-0022 Staged Service Plane Delivery.
- Dependency 2: AR-0006 CEO/Exec Runtime Model.
- Dependency 3: ER-0001 Service Model and IPC Foundation.
- Dependency 4: ER-0034 Referee Storage Layout Implementation (v1).

## Implementation Notes

- Notes for implementer: keep the first host lifecycle state machine minimal and explicit; do not couple restart semantics to Conch session state.

### Implementation Design (2026-10-13)

The System Engineer authorized implementation of the missing lifecycle and persistent registry
after the repository audit found no matching implementation for the existing Verified status.
That historical status is not evidence that the acceptance criteria currently pass.

- Add local service-record and service-host modules under `src/services/`, using the existing
  Referee storage API and CEO `TaskMode::Service` tasks.
- Persist the complete descriptor, including endpoint identities and required grants. Runtime
  handler pointers and process-local task IDs must not be treated as durable executable bindings.
- Keep service identity stable through create, start, stop, restart, and store reopen.
- Record lifecycle changes as new immutable records. The current store's
  `create_object_with_id` always uses version 1, so it must not be used to overwrite lifecycle
  history under a reused ObjectID.
- Expose persisted discovery and host state through Conch, with control paths consuming the
  command AST. Keep implementation details outside command output unless needed for inspection.
- The System Engineer selected explicit binding and start. Recovery restores descriptors with
  stopped, unbound hosts. Automatic restart policy is deferred.

### Validation Targets

- Close and reopen a file-backed store; verify name/type discovery, stable identity, endpoint
  metadata, and required grants survive.
- Verify legal create/start/stop/restart transitions and rejection of invalid transitions.
- Verify requests are routed only to a live host and resume after restart.
- Verify reopened hosts cannot reuse a stale task ID or handler pointer.
- Verify duplicate identities/names/types, malformed records, and storage failures return typed
  errors without publishing a partially recovered runtime registry.
- Verify Conch inspection and lifecycle commands across separate shell processes.
- Run the focused service, capability, CEO, parser, and shell tests, followed by `make check`.
- Independent Live Pass: Pending; the System Engineer must observe service lifecycle and
  rediscovery through Conch before verification is recorded.

### Implementation and Validation Evidence (2026-10-13)

- `src/services/host_records.{h,cc}` persists full descriptors and immutable, predecessor-linked
  lifecycle history. `service_host.{h,cc}` implements CEO service tasks, explicit recovery/binding,
  lifecycle controls, and health checks. `service_commands.{h,cc}` exposes the built-in memory
  service through typed Conch commands; existing IPC capability enforcement remains in the path.
- `tests/test_service_host.cc` passes all six tests, including file-backed recovery, name/type
  routing, illegal transitions, rejected bindings, duplicate descriptors, corruption, externally
  terminated tasks, capability enforcement, and failed storage writes. The Conch authoring suite
  passes the new two-process recovery/binding/start/request test.
- `./bootstrap.sh`, `./configure`, and `make -j4` passed. Regeneration was necessary to include the
  new source modules and test target. Generated tracked outputs are excluded from the change.
- The focused eight-program suite passed with
  `TMPDIR=/private/tmp make -C tests check TESTS='test_service_host test_service_ipc test_capability_context test_ceo_tasks test_conch_parser test_conch_grammar_api test_conch_parser_regression test_conch_authoring'`.
- `TMPDIR=/private/tmp make check` passed 31/31. Both suites required execution outside the sandbox:
  sandbox runs failed before assertions because Check could not create its communication files.
- The previous Verified label is replaced with Complete. Independent Live Pass and System Engineer
  verification remain Pending; no historical verification is used to infer either gate.
- Operating limits and a runnable Conch sequence are documented in `docs/Service-Hosts.md`.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect service descriptors before and after restart.
  - start, stop, and restart a task-backed service host and verify rediscovery.
