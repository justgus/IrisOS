# IrisOS v0 Architecture Memo

Object-native OS with Conch shell, CEO tasks, and Erector standard objects

## Goal

Boot a machine into a terminal-like environment where the user performs object operations (not file/program commands). Objects are persisted in Referee (object graph), executed under CEO (task system), introspected through Refract (reflection), and visualized organically in Conch via Vizier artifacts (spawned “Conchos” panes).

---

## 1) Canonical subsystem map

### Runtime pillars

- Referee — persistent object graph (Object store + Relationship store + indexes + transactions)
- CEO — task runtime (spawn/schedule/supervise/kill/cancel; wait/wake integration)
- Conch — object shell session + UI surface (terminal-first, pane/tiling capable)

### Erector (standard object universe)

Erector is a library of types and contracts (objects + operations), not “a service.”

- Erector::Core — primitives (Bool, Int, Float, String, List, Map, ObjectRef, Result, etc.)
- Erector::Math — Vector, Matrix, Tensor, etc.
- Erector::Refract — reflection/meta-model (Types, Classes, Definitions, Operations…)
- Erector::Machine — hardware-facing descriptors + capabilities (cores, memory regions, registers, timers, interrupt lines…)
- Erector::Exec — concurrency primitives as objects (Mutex, Semaphore, Event, Channel<T>, Future<T>, Deadline)
- Erector::Comms — communication stack (serial, USB endpoints, sockets, protocols)
- Erector::Vizier — declarative “viewable artifacts” (logs/metrics/tables/trees/streams)

Rule of thumb:
Erector defines objects and contracts. Referee persists them. CEO executes them. Conch renders and orchestrates them.

---

## 2) Refract core model: Class, Definition, Instance

You called out the critical distinction: Definition vs Instance.

### Concepts

- Class: the conceptual OO type (inheritance, polymorphism).
- Definition: a specific versioned contract of a Class (fields/ops/signatures/constraints), immutable-ish once published.
- Instance: a stored object that references the Definition it conforms to.

### Why Instances should reference Definitions

Schema evolution becomes sane:

- Widget@v3 and Widget@v7 are both “Widget,” but you can validate, migrate, and interpret them correctly.
- Referee can enforce constraints based on the referenced Definition.
- Conch can explain behavior precisely (“this instance conforms to WidgetDefinition v3”).

### Minimal relationships

- ClassDefinition --defines--> Class
- Instance --instanceOf--> Definition
- Definition --supersedes--> Definition (version chain)
- Definition --hasField--> FieldDefinition
- Definition --hasOperation--> OperationDefinition
- Definition --implements--> InterfaceDefinition (optional v0)
- Definition --extends--> Definition (inheritance as “definition extends definition”)

---

## 3) Refract objects (v0 minimal field list)

### Refract::Type

- TypeID (stable, not typeid().name())
- name (human)
- kind (primitive/class/interface/enum/union)
- namespace
- doc (optional)

### Refract::Class : Type

- baseClass / bases[] (TypeID or DefinitionID, depending on policy)
- polymorphic flag
- final flag (optional)

### Refract::Definition

- DefinitionID (ObjectID)
- classTypeID
- version (monotonic or semver-ish)
- fields[] (FieldDefinition refs)
- operations[] (OperationDefinition refs)
- relationshipSpecs[] (RelationshipSpec refs)
- supersedes (Definition ref)
- doc

### Refract::FieldDefinition

- name
- type (TypeRef)
- required bool
- defaultValue (Value)
- constraints (range/regex/enum/etc; keep minimal v0)

### Refract::OperationDefinition

- name
- signature (Signature ref)
- effects (pure | mutates | producesArtifacts | spawnsTasks | io)
- requiredCapabilities (optional v0)
- doc
- produces[] (optional but very useful for UI routing)

### Refract::Signature

- params[]: (name, TypeRef, optional?, variadic?)
- returns: TypeRef (or Result)
- async flag (or returns Future)

### Refract::RelationshipSpec

- roleName (string, optional)
- edgeName (string, optional)
- targetType (TypeRef)
- cardinality (1, ?, *, +)
- constraints (optional v0)

---

## 4) Referee: persistent object graph requirements (v0)

### Storage envelope

Each stored object should have:

- ObjectID
- TypeID + DefinitionID (or just DefinitionID if it implies Class/Type)
- payload (blob)
- metadata (created, updated, owner/caps hooks, tags)

### Relationships

- Store as separate edges: (fromID, toID, name?, role?, metadata)
- Index:
  - by fromID
  - by toID
  - by (fromID, name/role)
  - by TypeID / DefinitionID

### Transactions

- Atomic creation/update of:
  - object + edges + indexes
- Crash safety (WAL or similar)

---

## 5) CEO: tasks, supervision, kill (v0)

### Task model

- TaskID
- state (Ready/Running/Blocked/Done/Failed/Killed)
- parentTaskID (supervision tree)
- objectContext (ObjectID this task is “running for”)
- mailbox / event queue

### Minimum operations

- spawn(taskFn, context) -> TaskID
- cancel(TaskID) / kill(TaskID)
- await(Waitable) (Exec integration)
- ps() list tasks
- structured cancellation propagation (parent → children unless detached)

### Integration rule

- Exec primitives never “block a thread”; they request CEO to park the current task until a wake event occurs.

---

## 6) Vizier + Conch: organic “Quix boxes” UI growth

### The pattern

Execution emits objects; Conch subscribes to the graph.

When an object runs, it may create Vizier artifacts and link them to the running task/object. Conch watches for these edges and spawns Conchos.

### Vizier artifact types (v0)

- Vizier::Panel (base, rarely used directly)
- Vizier::TextLog (append-only stream)
- Vizier::Metric (name + numeric value + optional history)
- Vizier::Table (columns + rows; append/update)
- Vizier::Tree (nodes + edges; for object graph subsets)
- Vizier::Stream (bytes or typed messages; optional v0)

### Standard artifact edges (contract)

- Task/Object --produced--> Vizier::<Artifact>
- Optionally:
  - Task --progress--> Vizier::Metric
  - Task --diagnostic--> Vizier::TextLog
  - Task --result--> Vizier::Table (or result object)

### Conch subscription behavior

Conch session watches:

- new produced edges from the active Task/Object
- new child tasks (Task --spawned--> Task)
- optionally: artifacts produced by related objects involved in the operation

### Concho spawning rules (routing)

- If artifact is TextLog → create Log Concho
- If Metric → create Metric Concho
- If Table → create Table Concho
- If Tree → create Tree Concho
- If Task appears → create Task Monitor Concho

Routing can be declared in Refract (OperationDefinition.produces[] and/or Type preferredRenderer).

### Layout

Terminal-first tiling:

- Conchos are panes; panes can contain nested panes.
- New Conchos appear adjacent to the “parent” Concho (the task/object that caused them).

This is the “organic growth” demo moment.

---

## 7) Conch object shell command surface (v0)

Text UI, but object-typed semantics.

### Must-have commands

- ls (list reachable objects in current scope)
- show <ObjectID> (describe via Refract)
- edges <ObjectID> (relationships)
- find type <TypeName> (query Type index)
- call <ObjectID> <opName> [args...] (validated via Refract signatures)
- start <ObjectID> (invokes start() op; spawns CEO task)
- ps / kill <TaskID>

### Piping (optional v0)

Eventually: pipelines of typed streams (ObjectRefs/Values), not raw text. For v0, you can fake it with structured output in the shell and add true typed piping later.

---

## 8) Killer demo script (v0 concrete)

Boot → Conch. User runs:

1. find type MapGenerator
2. start <MapGeneratorObjectID>

Immediately Conch spawns:

- Task Monitor Concho: status + TaskID

As execution progresses, the running object publishes:

- Vizier::TextLog (“generating seed…”) → Log Concho appears
- Vizier::Metric (progress=0.12) → Metric Concho appears
- Vizier::Table (parameters chosen, timings) → Table Concho appears
- Task --spawned--> TextureSynthTask → nested Task Concho appears
- Vizier::Tree of produced MapTile objects → Tree Concho appears

User can then:

- edges <TaskID> (or task object) and see exactly why these Conchos exist (because the artifacts are objects linked in Referee).
- kill the child task and watch the tree/panels update.

Punchline: the UI grows because the object graph grows, not because the original program “printed a UI.”

---

## 9) v0 build boundary (what not to do yet)

### Not in v0

- Full GUI compositor
- Full TCP/IP stack (you can stub Comms)
- Full driver model, real interrupts, real registers
- Advanced security/capabilities enforcement (hooks only)
- Distributed object linking/sync

### In v0 (minimum to ship the demo)

- Referee object + edge store, with Type/Definition indexing
- Refract minimal meta-model + registry
- CEO cooperative task runtime + kill + wait/wake
- Conch terminal with panes + subscriptions
- Vizier artifacts + routing rules
- 1–2 demo objects that publish artifacts dynamically

---

## 10) Naming checkpoint

- Object store: Referee
- Task runtime: CEO
- Standard object universe: Erector
- Reflection: Erector::Refract
- Concurrency objects: Erector::Exec
- Communication: Erector::Comms
- System/hardware descriptors: Erector::Machine
- Viewable artifacts: Erector::Vizier
- Shell: Conch (spawns Conchos)

---

## 11) Composite Summary with drill-down

### Core idea

The Demo Object publishes a Summary object plus optional Detail objects, all connected by edges.
Conch renders the summary first, then spawns Conchos as details appear or as the user expands.

### Standard objects (domain-agnostic)

- Vizier::Table — “current status snapshot”
- Vizier::Metric — key gauges (temps, RPM, requests/sec)
- Vizier::TextLog — diagnostics/events
- Vizier::Tree — object graph of components/subsystems
- Vizier::Stream — optional live feed

### Standard relationships

- DemoObject/Task --produced--> Vizier::*
- SummaryObject --summarizes--> ComponentObject
- ComponentObject --hasStatus--> StatusSample
- StatusSample --evidence--> RawReading (optional)
- Task --spawned--> Subtask for per-component analysis

### Variable detail levels

Implement as an operation argument and/or “expand edges”:

- start(detail=0|1|2|3)
- or user calls: call <summary> expand level=2

Conch can map “expand” to spawning nested Conchos.

### Option A: Ship Propulsion Status (“PropulsionSynth”)

Objects:

- PropulsionSystem (the Demo Object)
- Subsystem (Reactor/Drive/Power/ThrustVectoring/Cooling)
- Sensor / Reading
- Fault / Advisory
- Vizier::* artifacts

What Conch shows (organic growth):

1. Start PropulsionSystem
   - Concho: System Summary Table
   - Concho: Key Metrics (Thrust %, Core Temp, Vibration, Fuel Flow)
2. As it discovers subsystems:
   - Concho: Tree (Propulsion → Power → Cooling → …)
3. When anomalies are detected:
   - Concho: Fault List Table (severity, subsystem, recommended action)
   - Concho: Diagnostics Log
4. When the user expands a subsystem node:
   - spawn nested Conchos for that subsystem’s sensors + trend metrics

Detail levels (example):

- 0: green/yellow/red summary + 5 metrics
- 1: subsystem tree + fault list
- 2: per-subsystem sensor table + last N readings
- 3: raw evidence stream + algorithm trace (“why fault triggered”)

### Option B: Website Traffic Graph (“TrafficOracle”)

Objects:

- TrafficOracle (Demo Object)
- TimeSeries objects (per metric)
- Segment objects (country, campaign, page path)
- Anomaly objects
- Recommendation objects (optional)

What Conch shows:

1. Start TrafficOracle
   - Concho: Topline Metrics (Requests/min, Unique users, Errors %, P95 latency)
   - Concho: Time window summary table
2. When segments are computed:
   - Concho: Table “Top pages / referrers / geos”
3. If anomaly detection runs:
   - Concho: Anomaly table + drill-down links
4. When user selects an anomaly:
   - nested Conchos appear: “before/after series”, “suspect referrers”, “error logs”

Even in a text-first Conch, you can do “sparklines” in tables to make it pop.

### Option C: Algorithm Pipeline (“AlgorithmWorkbench”)

Objects:

- AlgorithmWorkbench (Demo Object)
- Pipeline → Stage → Run
- Dataset / Artifact objects
- Trace objects (per stage)
- PerfSample objects

What Conch shows:

1. Start Workbench
   - Concho: Pipeline overview table (stage, status, time, output)
2. Each stage spawns a child task:
   - nested Task Conchos appear automatically
3. Stages publish:
   - Vizier::Metric for throughput/memory
   - Vizier::TextLog for trace
   - Vizier::Table for results/params

This one is basically a live “graph of tasks” demo that matches your CEO lineage.

---

## 12) Schemas (Definitions) as First-Class Objects

### What “schema” means in IrisOS

A schema is not an external .json file. A schema is an object in Referee (persisted), described
by Refract, and referenced by Instances.

The non-negotiable trio:

- Class = conceptual type (“PropulsionSystem”)
- Schema / Definition = versioned contract (“PropulsionSystemDefinition v3”)
- Instance = stored object (“PropulsionSystem #A1B2…”) → points at Definition v3

This foundation enables reflection, validation, migrations/versioning, and polymorphism.

### What a schema must contain (v0 minimum)

Think of a schema as four sections:

1) Data shape
   - fields: name, type, required/optional, default, constraints

2) Behavior surface
   - operations: name, signature, effects, docs
   - (optionally) “produces Vizier artifacts” for UI routing

3) Relationship contract
   - allowed edges/roles, cardinality, target types

4) Serialization + versioning hooks
   - codec id (how to serialize payload)
   - migration path from previous schema versions

### Where schemas live in the architecture

Schemas live in Erector::Refract.

- Refract::Class
- Refract::Definition (schema)
- Refract::FieldDefinition
- Refract::OperationDefinition
- Refract::RelationshipSpec
- Refract::Signature

Referee stores them. CEO uses them for safe invocation. Conch uses them for discovery/UI.

### Two kinds of schema

A) Structural schema (instance layout)

“How is this object’s data structured?” This is the Definition + fields + serialization codec.

B) Interface schema (behavior + interaction)

“What operations can be invoked, with what args/return types, and what it may produce?”
This is operations + signatures + effects + relationship specs.

Your demo needs interface schemas even more than structural schemas, because Conch must discover
capabilities and render results.

### Representation options

Option 1 (recommended): Refract-native schemas

Schemas are stored as Refract objects in Referee. This is the “object OS” way.

Option 2: Dual representation (bootstrap)

Define schemas in C++ and mirror them into Referee as Refract objects at boot.

Best move: start with Option 2 for speed, but design toward Option 1 as the end state.

### Schema format

Internally, schema is Refract objects. Externally, schema can be exported/imported as JSON/YAML/TOML
for tooling and inspection.
