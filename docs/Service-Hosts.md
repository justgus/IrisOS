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
