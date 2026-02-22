---
GitHub-Issue: #69
---

# AR-0005 — Service Plane Model (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS is an object-oriented operating system. We need a consistent model for long-running
services that provide system-wide functions (e.g., IPC, networking, storage, memory, sandboxing).
The model must align with object identity and the Referee storage system, and must support
reliable inter-object communication. This model is built on top of the CEO/Exec runtime primitives
(AR-0006).

## Recommendation

Define the **Service Plane** as the set of Service Objects: Objects whose action continues
indefinitely and which provide a stable interface for data/communication to other Objects. The
system should standardize:

- **Service identity**: Service Objects have stable ObjectID and well-known TypeID.
- **Lifecycle**: Service Objects can be created, started, stopped, and restarted deterministically.
- **Messaging**: Objects communicate with services via a standardized message envelope and routing.
- **Discovery**: Objects discover services by name/type via a registry service.
- **Isolation**: Services operate in sandboxes with explicit capabilities and access policies.

### Service Object Contract

- **Interface**: Each service exposes one or more message endpoints (names or TypeIDs).
- **Message Envelope**:
  - sender ObjectID
  - recipient ObjectID or service endpoint
  - message type (TypeID)
  - payload (CBOR)
  - correlation id (ObjectID or UUID)
  - timestamp
- **Delivery Semantics**: At-least-once delivery within the local node, with explicit ACK/timeout
  semantics defined per service.

### Service Discovery

- A **Service Registry** maintains the mapping of service name/type to ObjectID and endpoint.
- Registry is a Service Object itself, with a minimal, stable API:
  - register(service)
  - unregister(service)
  - resolve(name/type)

### Core System Services (Initial Set)

- IPC / Inter-Object Communication service
- Referee (object store)
- Network service(s)
- Memory allocation service
- Sandbox / capability service

## Notes

This record defines the conceptual model. Implementation details (wire format, scheduler,
threading model, and persistence of registry state) should be covered by follow-on ERs.
