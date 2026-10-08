---
GitHub-Issue: N/A
AR-Dependencies: AR-0001
ER-Dependencies: ER-0081, ER-0082, ER-0083
---

# ER-0086 — Error Boundary Conformance

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass.

## ER Metadata

- ER ID: ER-0086
- Title: Error Boundary Conformance
- Status: In Progress
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0001 also requires expected-style public APIs, no exception-based normal control flow, no exceptions crossing module boundaries, and strongly typed namespaced error codes. Existing `Result<T>` and `ErrorCode` definitions are present, but the current AR-0001 gap review has not established conformance across public module boundaries.
- Background / constraints: ER-0081 through ER-0083 define missing-object lookup behavior. This ER audits and closes the remaining error-boundary requirements without replacing the project's existing `Result<T>` representation.

## Goals

- Inventory public cross-module APIs in the error-model layers covered by AR-0001, including Referee, Refract, Vizier, CEO, service, capability, and runtime boundaries.
- Ensure expected failures use `Result<T>` with typed, namespaced `ErrorCode` values and do not escape as exceptions across those boundaries.
- Keep exceptions out of normal lookup and absence control flow.

## Non-Goals

- Rewrite all internal exception use or remove exceptions from third-party libraries.
- Replace the current `Result<T>` implementation or change unrelated module APIs.
- Reclassify every historical error code throughout the repository.

## Scope

- In scope: public error-returning APIs and cross-module call paths in the named AR-0001 layers; corresponding focused tests and an API-to-failure-to-test inventory.
- Out of scope: unrelated subsystems outside AR-0001's error-propagation layers and private parsing/formatting details caught and translated before crossing a public module boundary.

## Requirements

- Functional: each in-scope public operation communicates expected failures through `Result<T>` and a strongly typed `ErrorCode` in its owning namespace.
- Functional: no exception escapes an in-scope public module boundary for an expected input, lookup, storage, or decoding failure.
- Functional: normal missing-object and optional no-data/no-route outcomes use the explicit result semantics defined in ER-0081 through ER-0083, not exceptions.
- Non-functional: successful behavior and error identity remain stable.

## API / Failure / Test Inventory

| Public API group | Expected failure distinction | Focused tests |
|---|---|---|
| `SqliteStore`: lifecycle, transactions, object create/read/list, edge query/mutation, graph cursor/change feed | Invalid state/input, persistence/corruption, and `NotFound` remain distinct; collection queries return empty collections when appropriate | `test_referee_core`, `test_conch_session_growth` |
| `SchemaRegistry`: registration, definition lookups, type listing, inheritance/supersedes queries; generic key/type derivation | Invalid definitions, missing records, malformed serialized data, and storage errors remain distinct | `test_refract_registry`, `test_refract_bootstrap` |
| `GenericRegistry` and `ScopedTypeRegistry`: register/get/find/resolve-or-register | Only `NotFound` permits lookup fallback/registration; validation and storage errors propagate | `test_refract_registry`, `test_conch_authoring` |
| `OperationRegistry::list_operations`, `refract::dispatch::resolve` | Missing owner/operation, invalid arguments, and schema/storage failures remain typed | `test_refract_registry`, `test_conch_authoring` |
| `ServiceRegistry`, `ServiceObject` authorization/request handling, and `MemoryService` registration/lookup/read/write | Missing service/region, authorization denial, invalid request, and storage errors remain distinct | `test_service_ipc` |
| `CapabilityContextStore`: persistence, get, list, and subject-filtered list APIs | Missing records, invalid payloads, and storage errors remain distinct; list absence stays an empty list | `test_capability_context` |
| `TaskRegistry`: task creation/spawn, state transitions, capability-context links, get/list | Missing tasks, invalid transitions/IDs, capability failures, and registry errors remain distinct | `test_ceo_tasks` |
| `TaskComms`: channel/datagram open and close for task pairs | Missing/invalid task endpoints and transport errors remain distinct | `test_ceo_tasks`, `test_comms_transport_session` |
| `IoReactor`: `await_readable` overloads for streams, channels, and datagram ports | Invalid handles, task-state failures, and transport errors remain distinct | `test_ceo_io_reactor` |
| `IoExecutor`: channel/datagram open, send, await, receive, and close operations | Invalid handles, capability/dispatch failures, transport errors, and normal no-datagram remain distinct | `test_ceo_io_reactor`, `test_comms_transport_session` |
| Vizier route, graph-change, and artifact-spawn APIs | Missing artifact and schema/storage failures remain distinct; no-route and unroutable graph changes remain successful empty optionals | `test_vizier_routing`, `test_conch_session_growth` |

The implementation audit records the public declarations in `referee_sqlite/sqlite_store.h`, `refract/schema_registry.h`, `refract/operation_registry.h`, `refract/dispatch.h`, `services/service.h`, `services/capability_context.h`, `ceo/task_registry.h`, `ceo/io_reactor.h`, and `vizier/routing.h`, any additional public cross-module error boundary discovered in those surfaces, and the test evidence for each expected failure. Any additional boundary found is added to this table before the ER can pass review.

## Proposed Approach

- Summary: enumerate the public error-returning APIs in the named AR-0001 layers, map each to expected failures and tests, then make only the boundary translations needed to satisfy the criteria. Extend focused tests to exercise each listed boundary and assert typed results rather than escaped exceptions.
- Alternatives considered: repository-wide exception removal was rejected because AR-0001 constrains public control-flow semantics and module boundaries, not every internal implementation technique.

## Acceptance Criteria

- The implementation inventory names each in-scope public boundary, its expected failure cases, its typed result, and the focused test that exercises those cases.
- Focused tests cover each inventory group with its applicable invalid-input, missing-object, storage, or decoding result; callback-capable boundaries also prove standard and non-standard exceptions are translated to `Internal`, and persisted schema decoding proves malformed data is translated to `CorruptData`.
- Tests prove expected lookup absence is represented by `ErrorCode::NotFound` and optional no-data/no-route remains a successful empty result where applicable.
- Existing focused tests for Referee, Refract, service/capability/task APIs, and Vizier routing pass.

## Risks / Open Questions

- Risk: this audit may expose a shared helper whose exception behavior affects more than one selected lookup path; changes must remain limited to those public boundaries.
- Question: none; the accepted AR-0001 error contract specifies the boundary behavior.

## Dependencies

- Dependency 1: ER-0081 Referee Object Lookup NotFound Semantics.
- Dependency 2: ER-0082 Refract Lookup NotFound Semantics.
- Dependency 3: ER-0083 Runtime Lookup NotFound Semantics.
- Dependency 4: AR-0001 Expected-Style Error Model.

## Implementation Notes

- Do not treat exceptions used internally by a dependency as violations when the owning public API catches and translates them.
- Preserve typed errors rather than converting them to `Unknown` during propagation.
- Implementation audit: public callback exceptions in Refract dispatch/operation inheritance and service request handling are translated to `Internal`; Vizier route lookup now returns and propagates schema listing failures; typed errors are preserved through the audited Referee SQLite, Refract, service, CEO, and Vizier call paths. Task and I/O boundary failures now distinguish invalid arguments, missing records, failed preconditions, and internal/storage errors.
- Test evidence: the focused service test covers timeout, missing service, and a non-standard thrown exception; Refract registry covers standard and non-standard inheritance resolver exceptions; task and I/O reactor tests assert `NotFound` and `FailedPrecondition` distinctions; phase 3 integration asserts a closed schema store returns `FailedPrecondition` through Vizier route lookup.
- Boundary evidence map: `test_referee_core` covers absent records and closed-store state; `test_refract_registry` covers missing/wrong-kind definitions, invalid registration, malformed stored definitions, and callback exceptions; `test_refract_bootstrap` covers schema bootstrap propagation; `test_service_ipc` covers registry absence/invalid input, authorization denial, memory absence, timeout, and callback exceptions; `test_capability_context` covers absent/wrong-kind contexts and invalid/duplicate payloads; `test_ceo_tasks` covers absent tasks, invalid transitions, and missing communication endpoints; `test_ceo_io_reactor` and `test_comms_transport_session` cover missing handles, no-data, and transport behavior; `test_vizier_routing` and `test_phase3_integration` cover missing artifacts, no-route, and schema storage failure; `test_conch_authoring` covers runtime propagation.

## Verification Plan

- Tests to run:
  - `make -j AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-dangling-gsl'`
  - `DYLD_LIBRARY_PATH=/Users/justgus/Xcode-Projects/IrisOS/src/.libs TMPDIR=/private/tmp make -C tests check TESTS='test_referee_core test_refract_registry test_refract_bootstrap test_service_ipc test_capability_context test_ceo_tasks test_ceo_io_reactor test_comms_transport_session test_vizier_routing test_conch_authoring'`
  - `DYLD_LIBRARY_PATH=/Users/justgus/Xcode-Projects/IrisOS/src/.libs TMPDIR=/private/tmp make check AM_CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -Wno-unused-const-variable -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments -Wno-dangling-gsl'`
- Manual checks: inspect the boundary inventory and verify it names each in-scope API and representative expected-failure test.

### Live Pass (Human)

- Required: No
- If no, reason: This work changes error propagation at API boundaries and is fully observable through deterministic automated tests; no visual or hardware behavior changes.
- Build / environment / target: N/A
- Steps: N/A
- Expected observations: N/A
- Observed results: N/A
- Result: Not Required
- Performed by: N/A
- Date: N/A
