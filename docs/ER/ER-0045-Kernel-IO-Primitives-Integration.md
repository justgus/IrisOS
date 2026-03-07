---
GitHub-Issue: #181
---

# ER-0045 — Kernel I/O Primitives Integration

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0045
- Title: Kernel I/O Primitives Integration
- Status: In Progress
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: kernel-grade execution requires I/O primitives that map cleanly to CEO tasks.
- Background / constraints: must integrate with Comms/Exec waitables and Conduit operations.

## Goals

- Define minimal I/O primitives for kernel task contexts.
- Ensure wait/wake integration with CEO for I/O operations.
- Provide task start/stop hooks aligned with CEO task lifecycle.
- Provide open/close hooks for inter-task communication channels.

## Non-Goals

- Full device driver stack.
- Advanced networking support beyond v1 needs.

## Scope

- In scope: minimal I/O primitives and CEO integration points.
- In scope: task start/stop and inter-task comms open/close primitives.
- Out of scope: real hardware drivers and interrupts.

## Requirements

- Functional: I/O operations can block/wake tasks deterministically.
- Functional: task start/stop and inter-task comms open/close are available to CEO callers.
- Non-functional: predictable and testable I/O behavior.

## Proposed Approach

- Summary: define minimal I/O primitives aligned with Comms and Exec waitables.
- Alternatives considered: defer I/O to userland (rejected for kernel demo).

## Acceptance Criteria

- I/O primitives integrate with CEO wait/wake loops.
- I/O operations can be invoked through Conduit.
- Start/stop and comms open/close hooks are callable in CEO.

## Risks / Open Questions

- Risk: I/O abstraction may not match eventual hardware requirements.
- Question: what minimal I/O set is required for v1 demo?

## Dependencies

- Dependency 1: ER-0015 Phase5 Comms Primitives.
- Dependency 2: ER-0016 Phase5 CEO I/O Reactor.

## Sub-ERs

- ER-0045.1 — Conduit I/O Operations and Schema Definitions.
- ER-0045.2 — Conch Commands and Invocation Verification.
- ER-0045.3 — End-to-End I/O Integration Tests.

## Implementation Notes

- Notes for implementer: keep abstractions minimal and avoid premature driver APIs.

## Implementation Plan

- ER-0045.1: Define minimal I/O operation schemas and Conduit execution plumbing.
- ER-0045.2: Expose Conch commands that invoke I/O operations end-to-end with capability checks.
- ER-0045.3: Build a full test matrix (unit, integration, and conch CLI) covering I/O wait/wake behavior.

### ER-0045.1 — Conduit I/O Operations and Schema Definitions

- Add schema definitions for kernel I/O primitives (channel, datagram, byte stream) and minimal task I/O operations.
- Define operation signatures for `open_channel`, `open_datagram`, `send`, `recv`, `await_readable`, `close`.
- Implement Conduit execution plumbing to map dispatch results to CEO I/O primitives (IoReactor and TaskComms).
- Provide explicit error mapping for closed channels, missing tasks, or invalid arguments.
- Add unit tests for schema registration, operation signature resolution, and Conduit execution dispatch.

### ER-0045.2 — Conch Commands and Invocation Verification

- Add Conch commands to create/open/close channels and datagram ports between tasks.
- Add Conch commands to send/recv bytes and await readability with proper task state transitions.
- Integrate capability checks on I/O operations (reuse ER-0044 hooks).
- Implement invocation verification in Conch: argument validation, operation resolution, and response shape checks.
- Add Conch-level tests that execute the new commands and verify successful invocation.

### ER-0045.3 — End-to-End I/O Integration Tests

- Add integration tests covering: open → send → await_readable → recv → close flows.
- Add tests for error cases (closed handles, missing task IDs, canceled tasks).
- Add stress tests for wait/wake behavior under repeated operations.
- Ensure the test suite runs via `make check` and is CI-friendly.

## Verification Plan

- Tests to run: I/O wait/wake integration tests.
- Manual checks: run a sample I/O task and observe task state transitions.
