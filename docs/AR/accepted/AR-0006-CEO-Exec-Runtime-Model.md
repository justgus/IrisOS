---
GitHub-Issue: #70
ER-Dependencies: ER-0005, ER-0006, ER-0007, ER-0016, ER-0017, ER-0043, ER-0044, ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4, ER-0051, ER-0057
---

# AR-0006 — CEO/Exec Runtime Model (Recommendation)

- Status: Implemented
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS requires a core runtime similar in spirit to AmigaOS Exec. This runtime ("CEO") provides
primitive services for task scheduling, message passing, resource ownership, and system-wide
coordination. Higher-level Service Objects (AR-0005) run on top of this runtime.

The runtime must support cooperative coroutines, OS threads, or a hybrid. All blocking semantics
should be expressed in terms of an abstract **Waitable/Awaitable** concept so that "blocking"
parks the current task rather than a specific OS thread.

## Recommendation

Define a **CEO/Exec Runtime** as the lowest-level object-oriented kernel layer with the following
responsibilities:

- **Tasking**: create/schedule/stop tasks and manage execution contexts.
- **Message Passing**: provide primitive queues/ports used by IPC services.
- **Resource Ownership**: track ownership and lifetimes of resources (memory, handles, capabilities).
- **Naming/Handles**: provide a stable handle model for kernel-managed objects.
- **Isolation Hooks**: expose capability checks and sandbox hooks used by higher services.

### CEO/Exec Primitives (Initial Set)

- Task control (spawn, yield, sleep, terminate)
- Waitable/Awaitable and `await(Waitable&)` for parking tasks
- Message ports and queues (send, receive, reply)
- Timers and deadlines
- Capability tokens (minimal, extensible)
- Basic memory allocation hooks

### Relationship to Service Objects and Exec

- CEO/Exec provides the primitive message-passing substrate.
- IPC/Service Registry are Service Objects built on CEO/Exec.
- CEO/Exec does not embed policy; policy lives in services.
- `Erector::Exec` provides synchronization primitives (Mutex/Semaphore/Event/Future) that implement
  Waitable/Awaitable using CEO primitives.

## Notes

This record defines the conceptual runtime model. Implementation details (data structures,
scheduling policy, threading model) should be defined in follow-on ERs.
