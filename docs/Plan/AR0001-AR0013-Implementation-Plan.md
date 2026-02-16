# Implementation Plan: AR-0001 through AR-0013

- Status: Draft
- Date: 2026-02-16
- Owners: Mike

## Scope

This plan implements the accepted Architecture Records (AR-0001 through AR-0013) and the IrisOS v0
architecture memo. It prioritizes a coherent v0 demo that proves the object graph + Conch UI model
while laying foundations for growth.

## Guiding Principles

- Build thin vertical slices that prove the end-to-end object graph pipeline.
- Prefer explicit contracts (Refract schemas) over implicit behavior.
- Keep v0 minimal: defer full networking, drivers, GUI compositor, and advanced policy.
- Treat SQLite as the current Referee backend to validate the object model quickly.

---

## Phase 0 — Repository Baseline and Docs (Done/Accepted)

### Features

- AR-0001 Error Model accepted.
- AR-0002 Object Identity accepted.
- AR-0003 Storage Layout Strategy accepted.
- AR-0004 Index and Graph Storage accepted.
- AR-0005 Service Plane accepted.
- AR-0006 CEO/Exec Runtime accepted.
- AR-0007 Refract Reflection Graph accepted.
- AR-0008 Erector Subsystems accepted.
- AR-0009 Referee Object Store accepted.
- AR-0010 Comms Subsystem accepted.
- AR-0011 Vizier Interpretation Layer accepted.
- AR-0012 Conch Shell and Conchos accepted.
- AR-0013 Viz Display Subsystem accepted.
- IrisOS v0 Architecture Memo added.

### Milestones

- M0.1: Architecture baseline established in docs.

### Risks

- Potential drift between docs and implementation if not aligned early.

### Tests

- None (documentation-only).

---

## Phase 1 — Core Schema + Referee Graph Baseline

### Features

1) **Refract schema objects (v0 minimal)**
   - Refract::Type, Class, Definition, FieldDefinition, OperationDefinition, Signature,
     RelationshipSpec.
   - Definition vs Instance linking.
   - Schema registry in Referee.

2) **Referee enhancements (SQLite)**
   - Persist objects with ObjectID + DefinitionID.
   - Persist edges with name/role metadata.
   - Indexes by TypeID/DefinitionID and edge name.

3) **Schema bootstrap**
   - Dual representation: schema defined in C++ and mirrored into Referee at boot.
   - Provide registry query API for Conch and CEO.

### Milestones

- M1.1: Minimal Refract schema registry stored in Referee.
- M1.2: Referee supports DefinitionID for objects + edge indexes.

### Risks

- Schema bootstrap can create duplication if registry and Referee drift.
- Over-designing the schema before Conch/CEO exercise it.

### Tests

- Unit tests for schema creation and lookup.
- Round-trip tests for Definition + Instance storage.
- Edge index tests for role/name queries.

---

## Phase 2 — CEO v0 Task Runtime + Exec Primitives

### Features

1) **CEO task model**
   - TaskID, states, parent/child supervision tree.
   - spawn, cancel, kill, await.
   - task context binds to ObjectID.

2) **Erector::Exec primitives**
   - Waitable/Awaitable abstraction.
   - Mutex, Semaphore, Event, Future (minimal).
   - CEO integration: await parks current task, not OS thread.

3) **Task registry object graph**
   - Tasks represented as objects for Conch/Vizier.

### Milestones

- M2.1: Task spawn/kill/await with supervision tree.
- M2.2: Waitable-based Exec primitives integrated with CEO.

### Risks

- Scheduling semantics may need revision once Conch and Comms stress concurrency.
- Cancellation propagation edge cases.

### Tests

- Task lifecycle tests (spawn, block, wake, cancel).
- Supervision tree propagation tests.
- Exec primitives test against CEO await.

---

## Phase 3 — Conch v0 Shell + Vizier + Viz

### Features

1) **Conch shell (terminal-first)**
   - ls, show, edges, find type, call, start, ps, kill.

2) **Vizier interpretation layer**
   - Routing rules based on type and Refract metadata.

3) **Erector::Viz artifacts**
   - Panel, TextLog, Metric, Table, Tree (v0).

4) **UI-as-object graph**
   - produced, progress, diagnostic edges.
   - Conch subscribes to graph and spawns Conchos.

### Milestones

- M3.1: Conch shell can list and inspect objects via Refract.
- M3.2: Conch auto-spawns Conchos for Viz artifacts.
- M3.3: Basic tiling layout with nested Conchos.

### Risks

- UI complexity could inflate; keep v0 minimal.
- Over-reliance on routing rules without good defaults.

### Tests

- Shell command tests for object lookup.
- Vizier routing tests.
- Conch subscription tests (object graph triggers view creation).

---

## Phase 4 — Demo Objects and Composite Summary Pattern

### Features

1) **Demo object (choose one)**
   - PropulsionSynth or TrafficOracle or AlgorithmWorkbench.

2) **Composite Summary pattern**
   - Summary object + detail objects linked by edges.
   - start(detail=N) expansion behavior.

3) **Artifact publishing**
   - TextLog, Metric, Table, Tree produced edges.

### Milestones

- M4.1: End-to-end demo: start object -> Conchos spawn organically.
- M4.2: Expand levels show deeper Conchos.

### Risks

- Demo scope creep.
- Data modeling complexity if demo object is too ambitious.

### Tests

- Demo scenario tests (golden output for Conch events).
- Artifact publication tests.

---

## Phase 5 — Comms v0 and CEO I/O Reactor

### Features

1) **Comms primitives**
   - Comms::Channel, ByteStream, DatagramPort.
   - SerialPort + TcpSocket stubs (loopback or in-memory).

2) **CEO I/O reactor**
   - Poller task that wakes Comms waitables.

3) **Integration**
   - Comms objects publish status artifacts.

### Milestones

- M5.1: ByteStream interface with in-memory loopback.
- M5.2: CEO reactor wakes Comms waitables.

### Risks

- OS-level I/O integration may be platform-specific.
- Reactor design may need refinement.

### Tests

- ByteStream send/recv tests.
- Reactor wake-up tests.

---

## Phase 6 — Hardening and Alignment

### Features

- Align Referee storage strategy with AR-0003/AR-0004 (decide on SQLite or segment/index).
- Expand test coverage (persistence, migrations, error handling).
- Document limitations and v1 roadmap.

### Milestones

- M6.1: Storage strategy decision captured in AR update if needed.
- M6.2: Test coverage baseline established.

### Risks

- Architectural divergence if SQLite remains without revisiting AR-0003/0004.

### Tests

- Persistence tests across restart.
- Migration tests for Definition versions.

---

## Open Questions

- Should schemas move from dual representation to Refract-native only in v1 or sooner?
- Should Conch adopt a pluggable renderer API early, or only once Viz objects stabilize?
- What minimum security/capability model is required for v0 demo?

## Deliverables Summary

- Working Conch shell with organic object-graph UI.
- CEO task runtime with wait/wake integration.
- Referee object graph with Refract schemas.
- Demo object proving the composite summary + drill-down pattern.
