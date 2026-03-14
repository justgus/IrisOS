# Implementation Plan

- Date: 2026-03-14
- Related report: [Gap-Analysis-2026-03-14.md](/home/justgus/Dev/irisOS/docs/reports/Gap-Analysis-2026-03-14.md)
- Scope: architecture corrections, implementation sequencing, test strategy, and ER/AR work needed to reduce or eliminate the gaps identified in the related gap analysis

## Purpose

This plan converts the findings in the related gap analysis into an actionable implementation program.

The goals are to:

- bring accepted architecture back into alignment with what can be implemented and verified incrementally
- expand the implementation scope in a controlled way rather than through large implicit jumps
- reduce the highest-risk architectural gaps first
- create a phase structure that fits the repository's existing AR/ER/DR workflow

## Planning Principles

- prefer staged architecture over over-broad "all at once" acceptance
- keep Referee and Refract as the persistent source of truth wherever practical
- preserve the existing working foundation in CEO, Referee, Refract, Conduit, and Conch
- make each phase independently testable and documentable
- introduce architecture amendments before or alongside the ERs that depend on them
- use DRs and regression tests as part of the delivery plan, not as an afterthought

## Recommended Architecture Modifications

These architecture modifications should be completed before or at the start of the implementation phases below.

### 1. Amend AR-0005 to define a staged Service Plane

Current issue:

- AR-0005 currently reads as though service lifecycle, registry persistence, isolation, and a broader service set are all part of the immediate architectural target.

Recommended modification:

- revise AR-0005 so that the Service Plane is explicitly staged:
  - Stage A: in-node service contract, registry, and message envelope
  - Stage B: service host lifecycle and deterministic restart behavior
  - Stage C: persistent registry state and service object records
  - Stage D: isolation and capability-aware service policy
  - Stage E: network, memory, and additional core system services

Reason:

- the current implementation already satisfies Stage A in part, so the AR should stop implying that the later stages are already part of the same verified surface.

### 2. Split AR-0008 into implementable Erector tracks

Current issue:

- AR-0008 groups `Erector::Math`, `Erector::Refract`, `Erector::Machine`, `Erector::Exec`, and `Erector::Comms` under one accepted recommendation, but only some tracks exist in code.

Recommended modification:

- keep AR-0008 as the umbrella direction but add follow-on ARs or amend it to define separate implementation tracks:
  - `Erector::Machine Core`: representation primitives and handles
  - `Erector::Machine Descriptors`: processors, memory, buses, devices
  - `Erector::Comms Realization`: hardware mapping above Machine

Reason:

- `Erector::Machine` is currently absent, so it needs its own architecture and ER sequence rather than remaining an implied side effect of Comms or CEO work.

### 3. Amend AR-0011 and AR-0012 to separate manual artifact routing from graph subscription

Current issue:

- the current AR language implies a graph-subscription architecture that does not yet exist.

Recommended modification:

- explicitly define two supported modes:
  - Mode 1: command-triggered or operation-triggered artifact-to-Concho routing
  - Mode 2: graph-observer-driven automatic subscription and view growth
- make Task visualization part of a later mode unless a concrete task view model is defined immediately

Reason:

- the current implementation already supports Mode 1, so the architecture should acknowledge that as a valid stepping stone instead of treating it as a partial deviation.

### 4. Amend AR-0007 to distinguish v1 core reflection from v2 full reflection

Current issue:

- AR-0007 presents Refract as if inheritance, interfaces, constraints, operation effects, permissions, and documentation are already first-class persisted entities.

Recommended modification:

- define a v1 core reflection profile containing:
  - types
  - fields
  - signatures
  - operations
  - relationships
  - generic instances
  - preferred renderer metadata
- define a v2 extended reflection profile containing:
  - base/interface relationships as authoritative graph data
  - constraints
  - effects
  - documentation objects
  - policy metadata beyond minimal capabilities

Reason:

- this removes ambiguity around which reflection features are required for near-term verification.

### 5. Amend AR-0015 to define parser maturity levels

Current issue:

- AR-0015 and ER-0023 imply a richer typed command AST and larger parser-driven refactor than currently exists.

Recommended modification:

- define parser maturity levels:
  - Level 1: quote-aware tokenization with basic command AST
  - Level 2: typed AST for built-in command families
  - Level 3: parser-driven validation and command dispatch
  - Level 4: reusable command grammar for userland tools and scripting surfaces

Reason:

- the current parser is a legitimate Level 1 implementation and should be documented as such.

### 6. Add an architecture note for defect and regression posture

Current issue:

- DR coverage is much thinner than the subsystem count and current complexity suggest.

Recommended modification:

- add a governance note or AR appendix requiring:
  - a DR for each verified regression or non-trivial user-visible defect
  - a linked regression test for each closed DR where practical
  - a periodic DR ledger review when a phase is marked complete

Reason:

- this creates a process correction alongside the implementation corrections.

## Phase Structure

The implementation work should proceed in seven phases. Each phase is scoped to produce reviewable ERs, code, and tests.

## Phase 0: Architecture Realignment And Planning Baseline

Objective:

- update the accepted architecture so that the target state is staged, explicit, and implementable

Work items:

- amend AR-0005, AR-0007, AR-0008, AR-0011, AR-0012, and AR-0015 as described above
- add a planning report index entry for this implementation plan and the gap report
- define the ER roadmap for the phases below
- identify which already-verified ERs need follow-on ERs versus errata only

Expected outputs:

- architecture amendments or follow-on ARs
- one roadmap-style governance or report document linking phases to future ER IDs

Exit criteria:

- each major gap has a corresponding future architecture target
- no phase below depends on architectural behavior that remains undefined

## Phase 1: Refract And Conduit Core Completion

Objective:

- close the highest-value reflection and dispatch gaps without destabilizing the working registry and bootstrap path

Scope:

- add persisted inheritance metadata to `TypeDefinition` and registry storage
- add persisted interface metadata or explicitly defer interfaces in a new AR amendment
- add operation effects metadata
- add field and relationship constraints metadata with validation hooks
- add documentation objects or a minimal documentation field model
- make dispatch use authoritative stored inheritance data instead of external-only resolver callbacks

Suggested ER sequence:

- ER: Refract inheritance and interface metadata
- ER: Refract constraints and validation metadata
- ER: Conduit effects and permission metadata expansion
- ER: Refract documentation objects and introspection

Code areas:

- [src/refract/schema_registry.h](/home/justgus/Dev/irisOS/src/refract/schema_registry.h)
- [src/refract/schema_registry.cc](/home/justgus/Dev/irisOS/src/refract/schema_registry.cc)
- [src/refract/bootstrap.cc](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc)
- [src/refract/operation_registry.h](/home/justgus/Dev/irisOS/src/refract/operation_registry.h)
- [src/refract/dispatch.cc](/home/justgus/Dev/irisOS/src/refract/dispatch.cc)

Tests required:

- schema round-trip tests for new metadata
- dispatch tests using persisted inheritance rather than ad-hoc callbacks
- Conch `show type` and operation listing regressions

Exit criteria:

- Refract becomes authoritative for the relationships that Conduit dispatch depends on
- the current gap around reflection breadth is materially reduced

## Phase 2: Conch Parser And Shell Decomposition

Objective:

- move Conch from token-driven special-case parsing toward typed AST-driven command handling

Scope:

- define a richer AST for command families:
  - type/schema commands
  - object commands
  - operation calls
  - task commands
  - IO commands
  - namespace/navigation commands
- refactor `conch.cc` so parsing and validation move out of the monolithic shell loop
- preserve existing behavior while shrinking command-specific ad-hoc parsing
- add parser coverage for spacing, quoting, key/value pairs, and aliases

Suggested ER sequence:

- ER: typed Conch AST and parser expansion
- ER: Conch command validation layer
- ER: Conch command-handler decomposition

Code areas:

- [src/parser/conch_command.h](/home/justgus/Dev/irisOS/src/parser/conch_command.h)
- [src/parser/conch_command.cc](/home/justgus/Dev/irisOS/src/parser/conch_command.cc)
- [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc)
- [tests/test_conch_parser.cc](/home/justgus/Dev/irisOS/tests/test_conch_parser.cc)
- [tests/test_conch_authoring.cc](/home/justgus/Dev/irisOS/tests/test_conch_authoring.cc)

Architecture impact:

- this phase should fulfill the revised Level 2 and Level 3 parser targets from the AR-0015 amendment

Exit criteria:

- Conch command parsing is AST-driven for the major built-in command families
- shell regressions are covered by parser and integration tests

## Phase 3: Service Plane Foundation Expansion

Objective:

- expand the current local service contract into a real service host and lifecycle foundation

Scope:

- add service host records and lifecycle states
- define deterministic create/start/stop/restart behavior
- persist service descriptors and registry bindings in Referee
- add task-backed service hosting through CEO
- define service health and restart semantics

Suggested ER sequence:

- ER: service host records and lifecycle state machine
- ER: persistent service registry and discovery
- ER: CEO-backed service hosting and restart policy

Code areas:

- [src/services/service.h](/home/justgus/Dev/irisOS/src/services/service.h)
- [src/services/service.cc](/home/justgus/Dev/irisOS/src/services/service.cc)
- [src/ceo/task_registry.h](/home/justgus/Dev/irisOS/src/ceo/task_registry.h)
- [src/referee_sqlite/sqlite_store.cc](/home/justgus/Dev/irisOS/src/referee_sqlite/sqlite_store.cc)
- Conch service inspection and control paths in [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc)

Tests required:

- registry persistence tests
- restart and lifecycle tests
- service discovery and request routing tests across service restarts

Exit criteria:

- the Service Plane moves beyond "in-process registry plus synchronous handler"
- services can be represented, inspected, and restarted deterministically

## Phase 4: Capability Context, Policy Plumbing, And Service Isolation

Objective:

- replace Conch-local capability grants with persistent capability context and service-aware policy hooks

Scope:

- define capability context objects in Referee/Refract
- bind capabilities to CEO task execution and service call paths
- replace shell-only grant state with object-backed or task-backed capability contexts
- add sandbox identifiers and capability checks at service boundaries
- keep policy minimal, but make the context real

Suggested ER sequence:

- ER: capability context objects and persistence
- ER: CEO task capability attachment
- ER: service boundary capability enforcement
- ER: sandbox identity and service isolation hooks

Code areas:

- [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc)
- [src/ceo/task_registry.h](/home/justgus/Dev/irisOS/src/ceo/task_registry.h)
- [src/refract/schema_registry.h](/home/justgus/Dev/irisOS/src/refract/schema_registry.h)
- [src/services/service.cc](/home/justgus/Dev/irisOS/src/services/service.cc)

Architecture impact:

- this phase closes the most visible mismatch between the accepted isolation model and the actual shell behavior

Exit criteria:

- capability checks no longer rely on shell-local string sets
- service calls and task execution can be evaluated against a persistent capability context

## Phase 5: Vizier, Conch, And Graph Observation

Objective:

- evolve the current artifact-routing implementation into a real graph-observer model

Scope:

- define graph observation APIs over Referee relationships
- introduce event or polling-based graph watch support
- support typed routing rules based on:
  - artifact type
  - producer relationship
  - task state
  - diagnostic or progress edges
- add task view models and Task Conchos explicitly
- move Concho creation from helper calls toward observer-driven creation

Suggested ER sequence:

- ER: Referee graph watch and change-feed API
- ER: Vizier relationship-pattern routing
- ER: task visualization objects and Task Conchos
- ER: observer-driven Conch session growth

Code areas:

- [src/vizier/routing.cc](/home/justgus/Dev/irisOS/src/vizier/routing.cc)
- [src/viz/artifacts.h](/home/justgus/Dev/irisOS/src/viz/artifacts.h)
- [src/conch_shell/conch.cc](/home/justgus/Dev/irisOS/src/conch_shell/conch.cc)
- Referee storage/query surfaces under [src/referee_sqlite/sqlite_store.cc](/home/justgus/Dev/irisOS/src/referee_sqlite/sqlite_store.cc)

Tests required:

- routing tests for task objects and relationship-pattern triggers
- integration tests showing observer-driven Concho growth
- persistence and replay tests for Conch session visualization state

Exit criteria:

- Vizier no longer depends solely on explicit artifact helper calls
- Task Concho routing exists as a real tested surface

## Phase 6: `Erector::Machine` And Comms Expansion

Objective:

- introduce the missing Machine subsystem and use it to ground the next Comms expansion

Scope:

- implement representation primitives first:
  - bytes, words, addresses, blobs, slices, packets, checksums, UUID-like primitives where needed
- implement machine/resource descriptors second:
  - processor
  - memory region
  - device
  - bus
- implement handles and leases third:
  - memory handle
  - device handle
  - IO region or packet port handles
- expand Comms above Machine with explicit transport/session and protocol objects

Suggested ER sequence:

- ER: Machine representation primitives
- ER: Machine descriptors and resource facts
- ER: Machine handles and leases
- ER: Comms transport/session objects
- ER: Comms protocol objects and hardware mapping

Code areas:

- new `src/machine/` module
- [src/comms/primitives.h](/home/justgus/Dev/irisOS/src/comms/primitives.h)
- [src/ceo/io_reactor.h](/home/justgus/Dev/irisOS/src/ceo/io_reactor.h)
- [src/refract/bootstrap.cc](/home/justgus/Dev/irisOS/src/refract/bootstrap.cc)
- [src/Makefile.am](/home/justgus/Dev/irisOS/src/Makefile.am)

Architecture impact:

- this phase should be treated as a deliberate architecture expansion, not just a coding backlog item

Exit criteria:

- `Erector::Machine` exists as a real module and schema surface
- Comms can reference machine-backed descriptors rather than remaining only loopback primitives

## Phase 7: Units Runtime, Defect Posture, And Verification Hardening

Objective:

- close medium-severity functional gaps and improve long-term quality posture

Scope:

- add a runtime unit conversion layer that consumes existing Caliper metadata
- define compatibility and conversion evaluation APIs
- expand DR coverage expectations
- add regression tests for each defect fixed during the earlier phases
- add phase review checklists for:
  - architecture alignment
  - ER completeness
  - DR coverage
  - test coverage

Suggested ER sequence:

- ER: Caliper runtime conversion engine
- ER: DR-linked regression harness expansion
- ER: phase completion verification checklist and tooling

Code areas:

- Caliper and Refract bootstrap and runtime support
- Conch inspection and conversion commands
- tests and governance documents

Exit criteria:

- unit conversion is executable behavior rather than only metadata
- DR and regression coverage are materially improved

## Cross-Cutting Workstreams

These workstreams should run throughout the phases above.

### Workstream A: Schema Migration Compatibility

- every new Refract metadata field must include deterministic persistence and migration handling
- bootstrap and recovery behavior must remain explicit and testable

### Workstream B: Conch UX Stability

- keep command compatibility stable wherever practical
- document syntax changes or validation tightening in the corresponding ER

### Workstream C: Test Layering

- add unit tests for each new metadata type
- add integration tests for each major subsystem boundary
- add regression tests for any closed DR

### Workstream D: File And Module Decomposition

- avoid adding more large monoliths
- use new local modules when expanding `conch.cc`, Refract metadata handling, or service runtime logic

## Recommended Delivery Order

The recommended order is:

1. Phase 0
2. Phase 1
3. Phase 2
4. Phase 3
5. Phase 4
6. Phase 5
7. Phase 6
8. Phase 7

Rationale:

- Phase 1 and Phase 2 strengthen the metadata and command foundation needed by the later service, visualization, and Machine work.
- Phase 3 and Phase 4 close the most operationally important architecture gaps.
- Phase 5 depends on stronger graph and task semantics.
- Phase 6 is intentionally later because it expands scope substantially and depends on clearer service, capability, and reflection boundaries.

## Priority Matrix

Highest priority:

- Phase 0: architecture realignment
- Phase 1: Refract and Conduit core completion
- Phase 2: Conch parser and shell decomposition
- Phase 3: Service Plane foundation expansion

Second priority:

- Phase 4: capability context and isolation
- Phase 5: graph observation and task visualization

Third priority:

- Phase 6: Machine and broader Comms expansion
- Phase 7: runtime unit conversion and defect-process hardening

## Definition Of Success

This plan succeeds when:

- the accepted ARs describe a staged, implementable target rather than an ambiguous aggregate of current and future behavior
- the Service Plane is persistent, lifecycle-aware, and capability-aware
- Refract becomes authoritative for the metadata Conduit and higher layers actually depend on
- Conch uses a richer typed parser and no longer relies on most ad-hoc command parsing
- Vizier and Conch support observer-driven graph visualization rather than only explicit helper invocation
- `Erector::Machine` exists as a real subsystem and anchors the next Comms expansion
- DRs and regression tests become part of the normal completion criteria for each phase

## Immediate Next Steps

- create the architecture amendments identified in Phase 0
- draft the first ER tranche for Phase 1 and Phase 2
- treat `conch.cc` decomposition, persisted inheritance metadata, and service lifecycle modeling as the first concrete implementation wave
