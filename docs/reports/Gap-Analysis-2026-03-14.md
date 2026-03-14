# Gap Analysis Report

- Date: 2026-03-14
- Scope: Accepted ARs, verified ERs, verified DRs, implementation sources, and existing tests
- Basis: static audit of repository content; no code changes or behavior changes included in this report

## Executive Summary

The codebase materially implements the Referee, Refract, CEO, Conduit, parser, and Conch foundations, but the accepted architecture is still ahead of the verified implementation in several important areas.

The largest gaps are:

- the full Service Plane model
- the absence of `Erector::Machine`
- the graph-driven Conch and Vizier model
- the breadth of the Refract reflection model
- the parser architecture promised by AR-0015 and ER-0023

By contrast, Referee persistence, Refract schema registration, Conduit dispatch, and CEO runtime primitives are materially present and reasonably covered by tests.

## Findings

### 1. Service Plane breadth still exceeds the implemented runtime

Severity: High

Accepted architecture:

- [docs/AR/accepted/AR-0005-Service-Plane-Model.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0005-Service-Plane-Model.md) defines deterministic lifecycle, standardized messaging, discovery, isolation, and an initial core service set including networking, memory allocation, and sandbox/capability services.

Implementation state:

- [src/services/service.h](/home/justgus/Dev/irisOS/src/services/service.h) and [src/services/service.cc](/home/justgus/Dev/irisOS/src/services/service.cc) implement an in-memory `ServiceRegistry` and synchronous `IpcService`.
- [tests/test_service_ipc.cc](/home/justgus/Dev/irisOS/tests/test_service_ipc.cc) verifies register, resolve, unregister, and request/response behavior.
- [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc) currently performs capability checks using a session-local `std::set<std::string>`.

Gap:

- no persistent registry state
- no deterministic create/start/stop/restart service lifecycle model
- no service process model beyond in-process handler dispatch
- no separate network, memory allocation, or sandbox/capability services
- no real capability context beyond Conch-local string grants

Supporting references:

- [docs/AR/accepted/AR-0005-Service-Plane-Model.md#L24](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0005-Service-Plane-Model.md#L24)
- [docs/AR/accepted/AR-0005-Service-Plane-Model.md#L53](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0005-Service-Plane-Model.md#L53)
- [src/services/service.h#L54](/home/justgus/Dev/irisOS/src/services/service.h#L54)
- [src/services/service.cc#L55](/home/justgus/Dev/irisOS/src/services/service.cc#L55)
- [src/conch_shell/conch.cc#L1518](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc#L1518)
- [docs/ER/ER-0046-v1-Kernel-Demo-Integration-and-Roadmap.md#L101](/home/justgus/Dev/irisOS/docs/ER/ER-0046-v1-Kernel-Demo-Integration-and-Roadmap.md#L101)

### 2. `Erector::Machine` is not present in the codebase

Severity: High

Accepted architecture:

- [docs/AR/accepted/AR-0008-Erector-Subsystems.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0008-Erector-Subsystems.md) defines `Erector::Machine` as a top-level subsystem for representation primitives, machine/resource descriptors, and permission-bearing handles.

Implementation state:

- [src/Makefile.am](/home/justgus/Dev/irisOS/src/Makefile.am) lists the built modules.
- [src/refract/bootstrap.cc](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc) seeds core types for Refract, Crate, Astra, Caliper, Conch, Kernel::Io, Viz, and Demo.

Gap:

- no `machine` module
- no native `Erector::Machine` namespace or equivalent implementation surface
- no tests for machine descriptors, representation primitives, or machine-bound capability handles

Supporting references:

- [docs/AR/accepted/AR-0008-Erector-Subsystems.md#L19](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0008-Erector-Subsystems.md#L19)
- [docs/AR/accepted/AR-0008-Erector-Subsystems.md#L37](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0008-Erector-Subsystems.md#L37)
- [src/Makefile.am#L3](/home/justgus/Dev/irisOS/src/Makefile.am#L3)
- [src/refract/bootstrap.cc#L874](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc#L874)

### 3. Conch and Vizier only partially implement the accepted graph-driven UI model

Severity: High

Accepted architecture:

- [docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md) says Vizier observes the object graph and includes routing such as `CEO::Task` to Task Conchos.
- [docs/AR/accepted/AR-0012-Conch-Shell-and-Conchos.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0012-Conch-Shell-and-Conchos.md) says Conch subscribes to graph changes and spawns views from graph activity.

Implementation state:

- [src/vizier/routing.cc](/home/justgus/Dev/irisOS/src/vizier/routing.cc) routes by `preferred_renderer` and a small hardcoded set of `Viz::*` types.
- [src/viz/artifacts.h](/home/justgus/Dev/irisOS/src/viz/artifacts.h) defines `Panel`, `TextLog`, `Metric`, `Table`, and `Tree`.
- [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc) explicitly calls `maybe_spawn_concho(...)` from command handlers.
- [tests/test_phase3_integration.cc](/home/justgus/Dev/irisOS/tests/test_phase3_integration.cc) verifies artifact routing and Concho creation for `Viz::*` artifacts.

Gap:

- no general graph subscription mechanism was found
- no `CEO::Task` to Task Concho routing path was found
- routing remains artifact-type driven rather than relationship-pattern driven
- Concho spawning is explicit helper invocation, not a general graph observer pipeline

Supporting references:

- [docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md#L21](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md#L21)
- [docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md#L29](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md#L29)
- [docs/AR/accepted/AR-0012-Conch-Shell-and-Conchos.md#L20](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0012-Conch-Shell-and-Conchos.md#L20)
- [src/vizier/routing.cc#L12](/home/justgus/Dev/irisOS/src/vizier/routing.cc#L12)
- [src/vizier/routing.cc#L35](/home/justgus/Dev/irisOS/src/vizier/routing.cc#L35)
- [src/conch_shell/conch.cc#L1664](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc#L1664)
- [src/conch_shell/conch.cc#L4922](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc#L4922)
- [tests/test_phase3_integration.cc#L57](/home/justgus/Dev/irisOS/tests/test_phase3_integration.cc#L57)

### 4. The implemented Refract model is narrower than the accepted reflection architecture

Severity: High

Accepted architecture:

- [docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md) describes a persistent reflection graph that includes base types, implemented interfaces, constraints, side-effects, permissions, relationship constraints, and documentation.

Implementation state:

- [src/refract/schema_registry.h](/home/justgus/Dev/irisOS/src/refract/schema_registry.h) defines `FieldDefinition`, `OperationDefinition`, `RelationshipSpec`, `GenericInstance`, and `TypeDefinition`.
- [src/refract/operation_registry.h](/home/justgus/Dev/irisOS/src/refract/operation_registry.h) and [src/refract/dispatch.cc](/home/justgus/Dev/irisOS/src/refract/dispatch.cc) use an external inheritance resolver instead of authoritative inheritance/interface data stored in Refract.

Gap:

- no persisted base-type model in `TypeDefinition`
- no persisted implemented-interface model
- no field constraints model
- no relationship constraints model
- no operation effects metadata
- no documentation objects in the implemented schema model

Supporting references:

- [docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md#L20](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md#L20)
- [docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md#L24](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md#L24)
- [src/refract/schema_registry.h#L15](/home/justgus/Dev/irisOS/src/refract/schema_registry.h#L15)
- [src/refract/schema_registry.h#L38](/home/justgus/Dev/irisOS/src/refract/schema_registry.h#L38)
- [src/refract/schema_registry.h#L88](/home/justgus/Dev/irisOS/src/refract/schema_registry.h#L88)
- [src/refract/operation_registry.h#L10](/home/justgus/Dev/irisOS/src/refract/operation_registry.h#L10)
- [src/refract/dispatch.cc#L70](/home/justgus/Dev/irisOS/src/refract/dispatch.cc#L70)

### 5. The parser exists, but not yet as the typed AST-driven command model described in the architecture

Severity: Medium

Accepted and verified direction:

- [docs/AR/accepted/AR-0015-Conch-Parser-and-Syntax.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0015-Conch-Parser-and-Syntax.md) describes a typed command AST and parser-driven validation.
- [docs/ER/ER-0023-Conch-Parser-Implementation.md](/home/justgus/Dev/irisOS/docs/ER/ER-0023-Conch-Parser-Implementation.md) says `conch.cc` should consume the AST rather than ad-hoc token vectors.

Implementation state:

- [src/parser/conch_command.h](/home/justgus/Dev/irisOS/src/parser/conch_command.h) defines `CommandAst` as `name`, `args`, and `errors`.
- [src/parser/conch_command.cc](/home/justgus/Dev/irisOS/src/parser/conch_command.cc) performs a loose tokenization pass.
- [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc) does use `parse_conch_command(...)`, but still contains large command-specific parsing and dispatch logic.
- [tests/test_conch_parser.cc](/home/justgus/Dev/irisOS/tests/test_conch_parser.cc) verifies quoted arguments and unterminated-string errors.

Gap:

- no richer typed AST for command kinds, key/value pairs, or structured argument forms
- command semantics remain largely embedded in the monolithic shell implementation
- parser coverage is still narrow compared with the architectural grammar sketch

Supporting references:

- [docs/AR/accepted/AR-0015-Conch-Parser-and-Syntax.md#L27](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0015-Conch-Parser-and-Syntax.md#L27)
- [docs/ER/ER-0023-Conch-Parser-Implementation.md#L37](/home/justgus/Dev/irisOS/docs/ER/ER-0023-Conch-Parser-Implementation.md#L37)
- [src/parser/conch_command.h#L10](/home/justgus/Dev/irisOS/src/parser/conch_command.h#L10)
- [src/parser/conch_command.cc#L5](/home/justgus/Dev/irisOS/src/parser/conch_command.cc#L5)
- [src/conch_shell/conch.cc#L5515](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc#L5515)
- [tests/test_conch_parser.cc#L20](/home/justgus/Dev/irisOS/tests/test_conch_parser.cc#L20)

### 6. Comms implementation still matches the verified v0 primitive phase more than the accepted subsystem architecture

Severity: Medium

Accepted architecture:

- [docs/AR/accepted/AR-0010-Comms-Subsystem.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0010-Comms-Subsystem.md) defines link-layer, transport/session, protocol, and hardware-mapped comms objects.

Implementation state:

- [src/comms/primitives.h](/home/justgus/Dev/irisOS/src/comms/primitives.h) implements loopback `ByteStream`, `Channel`, and `DatagramPort`.
- [src/ceo/io_reactor.h](/home/justgus/Dev/irisOS/src/ceo/io_reactor.h) implements a thin reactor and handle store for those primitives.
- [tests/test_comms_primitives.cc](/home/justgus/Dev/irisOS/tests/test_comms_primitives.cc) and [tests/test_phase5_integration.cc](/home/justgus/Dev/irisOS/tests/test_phase5_integration.cc) cover in-memory behavior.

Gap:

- no transport/session objects such as `TcpSocket`, `UdpSocket`, or `TlsSession`
- no protocol objects such as `IPv4`, `IPv6`, `Arp`, or `DnsResolver`
- no hardware mapping through `Erector::Machine`
- no device-backed or network-backed implementations

Supporting references:

- [docs/AR/accepted/AR-0010-Comms-Subsystem.md#L23](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0010-Comms-Subsystem.md#L23)
- [docs/AR/accepted/AR-0010-Comms-Subsystem.md#L64](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0010-Comms-Subsystem.md#L64)
- [docs/ER/ER-0015-Phase5-Comms-Primitives.md#L24](/home/justgus/Dev/irisOS/docs/ER/ER-0015-Phase5-Comms-Primitives.md#L24)
- [src/comms/primitives.h#L18](/home/justgus/Dev/irisOS/src/comms/primitives.h#L18)
- [src/ceo/io_reactor.h#L13](/home/justgus/Dev/irisOS/src/ceo/io_reactor.h#L13)
- [tests/test_comms_primitives.cc#L14](/home/justgus/Dev/irisOS/tests/test_comms_primitives.cc#L14)

### 7. Units and quantities are implemented mainly as metadata, not as a full runtime conversion system

Severity: Medium

Accepted architecture:

- [docs/AR/accepted/AR-0017-Collections-Math-Units.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0017-Collections-Math-Units.md) calls for unit conversion rules and compatibility checks.

Implementation state:

- [src/refract/bootstrap.cc](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc) defines Caliper types and seeds a unit catalog with scale and offset metadata.
- [docs/ER/ER-0033-Caliper-Units-and-Quantities.md](/home/justgus/Dev/irisOS/docs/ER/ER-0033-Caliper-Units-and-Quantities.md) explicitly limits scope to schema definitions, conversion metadata, and basic validation.

Gap:

- conversion metadata exists, but no general runtime conversion engine was found
- operational conversion behavior remains mostly declarative at the schema/catalog level

Supporting references:

- [docs/AR/accepted/AR-0017-Collections-Math-Units.md#L80](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0017-Collections-Math-Units.md#L80)
- [docs/ER/ER-0033-Caliper-Units-and-Quantities.md#L44](/home/justgus/Dev/irisOS/docs/ER/ER-0033-Caliper-Units-and-Quantities.md#L44)
- [src/refract/bootstrap.cc#L365](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc#L365)
- [src/refract/bootstrap.cc#L1024](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc#L1024)

### 8. DR coverage is thin relative to the declared architecture surface

Severity: Medium

Observed state:

- [docs/DR/DR-Status.md](/home/justgus/Dev/irisOS/docs/DR/DR-Status.md) currently records a single verified defect.
- [docs/DR/DR-0001-Conch-Compare-Alias-Resolution.md](/home/justgus/Dev/irisOS/docs/DR/DR-0001-Conch-Compare-Alias-Resolution.md) describes a real Conch bug which is fixed in [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc).

Gap:

- the repository shows one documented, verified defect for a codebase with many subsystems and active architectural growth
- the process signal suggests DR tracking and regression documentation are lagging the implementation surface

Supporting references:

- [docs/DR/DR-Status.md#L1](/home/justgus/Dev/irisOS/docs/DR/DR-Status.md#L1)
- [docs/DR/DR-0001-Conch-Compare-Alias-Resolution.md#L73](/home/justgus/Dev/irisOS/docs/DR/DR-0001-Conch-Compare-Alias-Resolution.md#L73)
- [src/conch_shell/conch.cc#L4559](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc#L4559)

## Areas Of Strong Alignment

### Referee persistence and recovery

- [src/referee_sqlite/sqlite_store.cc](/home/justgus/Dev/irisOS/src/referee_sqlite/sqlite_store.cc) materially implements segment and index persistence, reload, and index rebuild behavior.
- [tests/test_phase6_persistence.cc](/home/justgus/Dev/irisOS/tests/test_phase6_persistence.cc) exercises persistence round-trips, object and edge recovery, and definition version lookup.

Key references:

- [src/referee_sqlite/sqlite_store.cc#L121](/home/justgus/Dev/irisOS/src/referee_sqlite/sqlite_store.cc#L121)
- [src/referee_sqlite/sqlite_store.cc#L437](/home/justgus/Dev/irisOS/src/referee_sqlite/sqlite_store.cc#L437)
- [src/referee_sqlite/sqlite_store.cc#L518](/home/justgus/Dev/irisOS/src/referee_sqlite/sqlite_store.cc#L518)
- [tests/test_phase6_persistence.cc#L61](/home/justgus/Dev/irisOS/tests/test_phase6_persistence.cc#L61)

### Refract schema registration, generics, and Conduit dispatch

- [src/refract/schema_registry.h](/home/justgus/Dev/irisOS/src/refract/schema_registry.h), [src/refract/bootstrap.cc](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc), and [src/refract/dispatch.cc](/home/justgus/Dev/irisOS/src/refract/dispatch.cc) provide a real schema/dispatch foundation.
- [tests/test_refract_registry.cc](/home/justgus/Dev/irisOS/tests/test_refract_registry.cc) exercises schema round-trips, supersedes chains, generic instance determinism, scoped type registry behavior, and dispatch resolution.

Key references:

- [src/refract/schema_registry.h#L134](/home/justgus/Dev/irisOS/src/refract/schema_registry.h#L134)
- [src/refract/bootstrap.cc#L874](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc#L874)
- [src/refract/dispatch.cc#L70](/home/justgus/Dev/irisOS/src/refract/dispatch.cc#L70)
- [tests/test_refract_registry.cc#L373](/home/justgus/Dev/irisOS/tests/test_refract_registry.cc#L373)

### CEO and Exec runtime primitives

- [src/ceo/task_registry.h](/home/justgus/Dev/irisOS/src/ceo/task_registry.h) implements task records, lifecycle state, trace and profile capture, and task comms.
- [src/exec/waitables.h](/home/justgus/Dev/irisOS/src/exec/waitables.h) provides `Event`, `Semaphore`, `Mutex`, and `Future`.
- [tests/test_ceo_tasks.cc](/home/justgus/Dev/irisOS/tests/test_ceo_tasks.cc) and [tests/test_exec_waitables.cc](/home/justgus/Dev/irisOS/tests/test_exec_waitables.cc) provide practical coverage of that substrate.

Key references:

- [src/ceo/task_registry.h#L89](/home/justgus/Dev/irisOS/src/ceo/task_registry.h#L89)
- [tests/test_ceo_tasks.cc#L13](/home/justgus/Dev/irisOS/tests/test_ceo_tasks.cc#L13)
- [tests/test_exec_waitables.cc#L12](/home/justgus/Dev/irisOS/tests/test_exec_waitables.cc#L12)

## Method

This report is based on:

- accepted ARs in `docs/AR/accepted/`
- verified ERs in `docs/ER/`
- verified DRs in `docs/DR/`
- implementation sources under `src/`
- existing tests under `tests/`

This was a static audit of repository content. No implementation changes were made as part of the analysis itself.

## Caveats

- This report compares architecture and engineering records against the documented implementation and tests present in the repository.
- It does not prove that a missing capability is absent from all possible runtime behavior, only that it is not represented clearly in the code and tests inspected.
- [docs/AR/accepted/AR-0014-Conch-Authoring.md](/home/justgus/Dev/irisOS/docs/AR/accepted/AR-0014-Conch-Authoring.md) is stored under `accepted/` but still marked `Status: Proposed`, so it was not treated as accepted architecture for this report.
