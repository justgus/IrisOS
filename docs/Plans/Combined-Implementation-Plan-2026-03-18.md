# Combined Implementation Plan

- Status: Draft
- Date: 2026-03-18
- Owners: Mike
- Sources:
  - [AR0014-AR0021-Implementation-Plan.md](/home/justgus/Dev/irisOS/docs/Plans/AR0014-AR0021-Implementation-Plan.md)
  - [AR0022plus-Implementation-Plan-v2.md](/home/justgus/Dev/irisOS/docs/Plans/AR0022plus-Implementation-Plan-v2.md)
  - [Implementation-Plan-2026-03-14.md](/home/justgus/Dev/irisOS/docs/Plans/Implementation-Plan-2026-03-14.md)

## Purpose

This plan consolidates the remaining unimplemented or not-yet-closed work after the 2026-03-18
status update. It carries forward only items that are still open after reconciling accepted ARs,
verified ERs, and the latest implementation planning report.

## Current Baseline

- AR-0014 and AR-0018 through AR-0026 are accepted.
- ER-0001 through ER-0054 are verified.
- ER-0056 and ER-0057 are verified.
- ER-0055 is complete and remains the only ER not yet marked verified.
- The earlier plan items that were listed as proposed in the older plan documents now under `docs/Plans/`
  (ER-0034 through ER-0053) have already been implemented and verified, so they are not repeated
  as future work in this document.

## Planning Principles

- Carry forward only work that remains open against the accepted architecture.
- Prefer follow-on ERs for new implementation slices rather than expanding older verified ER scope.
- Keep architecture staging explicit where AR-0022 through AR-0026 narrowed the near-term target.
- Preserve reviewable, testable phases that can land independently.

## Phase 1 - Close Current Reflection Baseline

Related architecture:

- AR-0023 Refract Reflection Profiles

Objectives:

- complete verification of ER-0055
- decide which v2 reflection items need follow-on ERs now versus explicit deferral
- keep Refract authoritative for reflection metadata used by Conduit and Conch

Remaining work:

- verify ER-0055 and close any review fallout
- define follow-on ERs only for still-missing v2 profile items:
  - constraints and validation metadata
  - operation effects metadata
  - documentation objects and richer introspection metadata

Exit criteria:

- ER-0055 is verified
- the remaining AR-0023 scope is either mapped to concrete ERs or explicitly deferred

## Phase 2 - Service Plane Stage C Through E

Related architecture:

- AR-0005 Service Plane Model
- AR-0022 Staged Service Plane Delivery

Objectives:

- build on ER-0057 to finish the next service-plane stages beyond the local substrate
- move from persistent registry plus lifecycle into persistent capability context and service-aware
  policy boundaries

Remaining work:

- define persistent capability context objects and task/service attachment
- enforce capability checks at service boundaries instead of Conch-local state
- add sandbox identity and service isolation hooks
- plan later-stage service classes such as network, memory, and additional core services

Suggested ER slices:

- ER-0058 - capability context objects and persistence
- ER-0059 - CEO task capability attachment
- ER-0060 - service boundary capability enforcement
- ER-0061 - sandbox identity and service isolation hooks
- ER-0062 - additional staged service implementations after isolation is stable

Exit criteria:

- service calls and task execution evaluate against persistent capability context
- later-stage service work is staged behind explicit ERs rather than implied by AR-0005 alone

## Phase 3 - Conch and Vizier Observer-Driven Interaction

Related architecture:

- AR-0011 Vizier Interpretation Layer
- AR-0012 Conch Shell and Conchos
- AR-0025 Conch and Vizier Interaction Modes

Objectives:

- preserve existing routed-artifact behavior while adding the missing observer-driven model
- make task visualization and graph-driven Concho growth explicit, testable surfaces

Remaining work:

- define graph watch or change-feed APIs over Referee relationships
- add relationship-pattern and task-state routing in Vizier
- add task view models and Task Conchos
- move Concho creation from helper-driven paths toward observer-driven session growth

Suggested ER slices:

- ER-0063 - Referee graph watch and change-feed API
- ER-0064 - Vizier relationship-pattern routing
- ER-0065 - task visualization objects and Task Conchos
- ER-0066 - observer-driven Conch session growth

Exit criteria:

- Mode 2 from AR-0025 exists as a tested implementation
- task visualization no longer depends on ad hoc helper behavior

## Phase 4 - Parser Reuse and Userland Grammar Surface

Related architecture:

- AR-0015 Conch Parser and Syntax
- AR-0026 Conch Parser Maturity Levels

Objectives:

- build on ER-0056 so the parser is reusable outside the interactive shell loop
- make the command grammar available to userland tools and batch execution

Remaining work:

- define reusable parser entry points for non-interactive consumers
- document grammar boundaries for batch or scripted execution
- add regression coverage for shared parser use across shell and tooling paths

Suggested ER slices:

- ER-0067 - reusable Conch grammar API
- ER-0068 - batch execution and non-interactive parser integration
- ER-0069 - shared parser regression harness

Exit criteria:

- Level 4 from AR-0026 is implemented or explicitly deferred with a narrower acceptance target

## Phase 5 - Machine and Comms Expansion

Related architecture:

- AR-0008 Erector Subsystems
- AR-0010 Comms Subsystem
- AR-0024 Erector Machine and Comms Delivery Tracks

Objectives:

- introduce the missing `Erector::Machine` layer explicitly
- ground future Comms work in machine-backed descriptors, handles, and transport/session objects

Remaining work:

- add machine representation primitives
- add processor, memory, bus, and device descriptors
- add machine handles and leases
- expand Comms transport, session, and protocol objects above Machine

Suggested ER slices:

- ER-0070 - Machine representation primitives
- ER-0071 - Machine descriptors and resource facts
- ER-0072 - Machine handles and leases
- ER-0073 - Comms transport and session objects
- ER-0074 - Comms protocol objects and hardware mapping

Exit criteria:

- `src/machine/` exists as a real subsystem
- Comms work is no longer limited to loopback-style primitives

## Phase 6 - Caliper Catalog Completion and Runtime Conversions

Related architecture:

- AR-0019 Caliper Unit Catalog

Objectives:

- finish the unit-catalog work that goes beyond the verified starter catalog
- convert units and dimensions from metadata-only content into runtime behavior

Remaining work:

- expand the canonical SI and imperial catalog
- define deterministic conversion chains and compatibility evaluation
- persist the canonical catalog versioning model and extension precedence rules
- expose unit listing, lookup, and conversion through Conch and runtime APIs

Suggested ER slices:

- ER-0075 - full Caliper catalog expansion
- ER-0076 - runtime conversion and compatibility engine
- ER-0077 - Conch conversion and inspection commands

Exit criteria:

- AR-0019 is fully represented by executable catalog and conversion behavior

## Cross-Cutting Work

- add DR-linked regression coverage for each non-trivial defect fixed in the phases above
- keep schema migration behavior explicit for every new persisted metadata field
- continue decomposing large subsystems into local modules instead of adding new monoliths

## Delivery Order

1. Verify ER-0055 and finalize the AR-0023 follow-on scope.
2. Finish Service Plane staging work under AR-0022.
3. Implement observer-driven Conch/Vizier behavior under AR-0025.
4. Add reusable parser surfaces for AR-0026 Level 4.
5. Implement Machine and Comms delivery tracks under AR-0024.
6. Complete the Caliper catalog and runtime conversion work under AR-0019.

## Validation

- `grep -RIn "Status: Proposed" docs/AR`
- `grep -nE "ER-0055|ER-0056|ER-0057" docs/ER/ER-Status.md`
- review this plan against the source plans and [Implementation-Plan-2026-03-14.md](/home/justgus/Dev/irisOS/docs/Plans/Implementation-Plan-2026-03-14.md)
