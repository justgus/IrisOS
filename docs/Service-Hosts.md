# Service Hosts

Conch can persist service descriptors and control local, task-backed service hosts. Each descriptor
retains its service identity, name, type, endpoints, and required capability grants across process
restarts. Discovery remains available while a host is stopped.

Create and run the built-in memory metadata service:

```text
service create memory
service bind memory
service start memory
service show memory
service request memory memory.list_regions
service restart memory
service stop memory
service unbind memory
```

Use `service list` to inspect all hosts. `service show <name>` reports current state, whether a
handler is bound, health, and the start generation. A healthy host has a bound handler and a live
Running CEO service task. Requests to a stopped host fail explicitly. Restart stops the old task
and starts a new task while retaining service identity and the current handler.

After reopening the same store, descriptors are rediscovered, but every host is stopped and
unbound. Use `service bind memory` and `service start memory` before sending requests. Binding
alone does not start the host. Automatic restart is outside this implementation.

Requests can include a quoted JSON payload:

```text
service request memory memory.register_region '{"id":"0123456789abcdef0123456789abcdef","name":"ram","kind":"ram","base":4096,"size":8192}'
service request memory memory.list_regions
```

The memory handler's region collection is process-local; host persistence preserves the descriptor
and lifecycle, not arbitrary handler data. Conch currently provides explicit bindings for the
built-in memory handler. Other local service implementations can use `ServiceHostManager` through
the C++ API.

## Runtime and persistence contract

- Call `recover()` once before creating or controlling hosts. Recovery validates all stored
  histories and duplicate descriptor bindings before publishing the registry.
- `create()` makes a Created host. `bind()` checks the handler's entire descriptor against the
  persisted descriptor. Stop before replacing or removing a binding.
- Hosts borrow handlers. A handler must outlive its binding; stop and unbind before destroying it.
- The manager, Referee store, and CEO registry use serialized calls. One manager owns bindings for
  a store; concurrent managers or writers are outside this local runtime contract.
- The initial immutable host record uses the service ObjectID. Later lifecycle records have new
  ObjectIDs, explicit sequence numbers, and predecessor references. Recovery uses these sequences,
  not timestamps or persisted task IDs. Historical Running records do not imply a live host.
- Requests use the existing IPC authorizer and persisted descriptor metadata. Lifecycle management
  is a trusted local management API; effect metadata does not grant request authority.
- A failed start never publishes a handler for routing. A failed stop persistence write still
  disables routing and stops the task; the returned error reports that durable history may lag
  runtime state. Recovery always requires explicit binding and start.
- Durability and partial-write behavior follow the existing Referee segment store. This layer does
  not introduce transactional guarantees beyond that store's contract.

## Validation

```sh
./bootstrap.sh
./configure
make -j4
TMPDIR=/private/tmp make -C tests check TESTS='test_service_host test_service_ipc test_capability_context test_ceo_tasks test_conch_parser test_conch_grammar_api test_conch_parser_regression test_conch_authoring'
TMPDIR=/private/tmp make check
```

The service-host suite checks recovery, legal and invalid transitions, preserved descriptor and
capability metadata, duplicate bindings, corrupt history, unhealthy tasks, and storage errors.
The Conch authoring suite checks explicit binding/start and routing across two shell processes.

## Owned allocation service (C++ binding)

`AllocationService(id, AllocationLimits{total_bytes, bytes_per_subject, allocation_count})`
implements Stage E owned process memory through the same local host and IPC APIs. Create its
`descriptor()`, bind the instance, then explicitly start the host. Conch binding remains the
memory metadata handler; applications bind the allocator through `ServiceHostManager`.

Set the request recipient to the service ID and select one declared endpoint by name or type.
`message_type` must equal that endpoint's operation type; if both endpoint selectors are supplied,
they must agree. Payloads are CBOR objects with exactly one argument:

| Endpoint | Message type | Payload |
|---|---|---|
| `allocation.allocate` | `kAllocationAllocateType` | `{"size": positive_integer}` |
| `allocation.inspect` | `kAllocationInspectType` | `{"id": allocation_hex_id}` |
| `allocation.release` | `kAllocationReleaseType` | `{"id": allocation_hex_id}` |

Responses use `kAllocationResponseType` and return `id`, `owner`, and `size`, preserving request
correlation and sandbox. Buffers are real zero-initialized owned storage; endpoints expose metadata,
not raw addresses or write access. Ownership follows the trusted local envelope sender. Other
subjects receive FailedPrecondition; missing/released IDs receive NotFound. Invalid arguments or
selectors return InvalidArgument; exhausted limits return FailedPrecondition; allocation failure
returns IoError. Zero limits disable allocation. Release restores quotas; handler destruction
releases all buffers. Calls are serialized. The subject quota scan is bounded by allocation_count.

Only the descriptor persists. Recovery is stopped/unbound; binding a fresh allocator instance
starts empty with limits supplied by the trusted application. Restarting the same handler retains
its buffers. Quotas are per handler instance, not a process-wide memory budget.

## Local Comms network service (C++ binding)

`NetworkService(id, schemas, leases, protocol, transport, maximum_sessions)` binds the existing
Machine-backed Comms loopback packet path to a ServiceObject. Protocol and transport are copied;
the schema registry, lease registry, and its borrowed inputs must outlive the handler. Bind and
start it through ServiceHostManager as above. It requires an authorized stream transport with a
compatible protocol. Descriptor-only transports cannot acquire session authority.

Every operation checks the envelope sender against the bound transport owner, revalidates its
Machine lease, and checks protocol compatibility. Selectors follow the allocator's strict contract.
Responses use `kNetworkResponseType` and preserve correlation, recipient, and sandbox.

| Endpoint | Message type | CBOR object fields |
|---|---|---|
| `network.open` | `kNetworkOpenType` | `first_endpoint`, `second_endpoint`: hex ObjectIDs |
| `network.inspect` | `kNetworkInspectType` | `id`: session hex ID |
| `network.round_trip` | `kNetworkRoundTripType` | `id`, `payload`: CBOR byte string, at most 1 MiB |
| `network.close` | `kNetworkCloseType` | `id`: session hex ID |

Metadata returns `id`, `transport`, `first_endpoint`, `second_endpoint`, and `state` (`open` or
`closed`). Round trip also returns the exact byte-string payload after registered packet framing,
loopback transport, and decoding. Packet schema registration through `bootstrap_core_schema`
is required before traffic. Closed sessions remain inspectable and count toward maximum_sessions;
zero disables new sessions. A fresh handler starts with no sessions. No session state persists.
Invalid requests publish no sessions; closure is staged until response construction succeeds.
Missing sessions return NotFound, authorization/lifecycle/limit failures FailedPrecondition,
invalid payload/selectors InvalidArgument, and allocation failure IoError. This is local loopback
service delivery; remote networking and hardware drivers remain future scope.
