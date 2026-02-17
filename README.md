# IrisOS
Object Oriented Operating System

## Service Model and IPC Foundation

Minimal in-process Service Object and IPC primitives live in `src/services/service.h`. Key pieces:

- `ServiceObject` interface with a `ServiceDescriptor` (ObjectID, TypeID, name, endpoints).
- `ServiceRegistry` for register/resolve/unregister by name or TypeID.
- `IpcService` for in-process request/response with correlation IDs and timeouts.
- `MessageEnvelope` helpers for constructing requests and responses.
