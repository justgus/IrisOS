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
- ER-0055 through ER-0064 are verified.
- ER-0078 is verified.
- The AR-0023 reflection-profile follow-on scope is mapped to ER-0078 through ER-0080.
- The earlier plan items that were listed as proposed in the older plan documents now under `docs/Plans/`
  (ER-0034 through ER-0053) have already been implemented and verified, so they are not repeated
  as future work in this document.

## Planning Principles

- Carry forward only work that remains open against the accepted architecture.
- Prefer follow-on ERs for new implementation slices rather than expanding older verified ER scope.
- Keep architecture staging explicit where AR-0022 through AR-0026 narrowed the near-term target.
- Preserve reviewable, testable phases that can land independently.

## Phase 1 - Close Current Reflection Baseline

- Status: Complete
- Completed: 2026-03-19

Related architecture:

- AR-0023 Refract Reflection Profiles

Objectives:

- complete verification of ER-0055
- decide which v2 reflection items need follow-on ERs now versus explicit deferral
- keep Refract authoritative for reflection metadata used by Conduit and Conch

Completed work:

- ER-0055 is verified.
- Review of the Refract schema and dispatch surfaces confirmed that authoritative inheritance
  and interface metadata were implemented, while constraints, operation effects, and documentation
  objects needed follow-on persisted reflection work.
- The remaining AR-0023 scope was mapped into concrete follow-on ERs:
  - ER-0078 - Refract constraints and validation metadata
  - ER-0079 - Refract operation effects metadata
  - ER-0080 - Refract documentation objects and richer introspection metadata

Exit criteria:

- ER-0055 is verified
- the remaining AR-0023 scope is either mapped to concrete ERs or explicitly deferred

## Phase 2 - Service Plane Stage C Through E

- Status: Verified
- Verified: 2026-05-28

Related architecture:

- AR-0005 Service Plane Model
- AR-0022 Staged Service Plane Delivery

Objectives:

- build on ER-0057 to finish the next service-plane stages beyond the local substrate
- move from persistent registry plus lifecycle into persistent capability context and service-aware
  policy boundaries

Completed work:

- ER-0058 defines persistent capability context objects and persistence.
- ER-0059 attaches capability context to CEO tasks.
- ER-0060 enforces service-boundary capability checks.
- ER-0061 adds sandbox identity and service isolation hooks.
- ER-0062 adds the memory service baseline.

Verified ER slices:

- ER-0058 - capability context objects and persistence
- ER-0059 - CEO task capability attachment
- ER-0060 - service boundary capability enforcement
- ER-0061 - sandbox identity and service isolation hooks
- ER-0062 - memory service baseline

Exit criteria:

- service calls and task execution evaluate against persistent capability context
- later-stage service work is staged behind explicit ERs rather than implied by AR-0005 alone

## Reflection Follow-On Phase - Remaining Profile Implementation

- Status: Proposed

Related architecture:

- AR-0023 Refract Reflection Profiles

Objectives:

- finish the remaining persisted reflection-profile features after ER-0078
- make operation effects and canonical documentation metadata queryable through Refract

Remaining work:

- add persisted operation-effects metadata for operation definitions
- add persisted documentation objects or documentation metadata for reflected entities
- keep schema migration behavior explicit for each new reflected metadata field

Verified ER slices:

- ER-0078 - Refract constraints and validation metadata

Proposed ER slices:

- ER-0079 - Refract operation effects metadata
- ER-0080 - Refract documentation objects and richer introspection metadata

Exit criteria:

- operation effects are persisted and exposed through introspection
- canonical Refract documentation metadata is persisted and exposed through introspection

## Phase 3 - Conch and Vizier Observer-Driven Interaction

Related architecture:

- AR-0011 Vizier Interpretation Layer
- AR-0012 Conch Shell and Conchos
- AR-0025 Conch and Vizier Interaction Modes

Objectives:

- preserve existing routed-artifact behavior while adding the missing observer-driven model
- make task visualization and graph-driven Concho growth explicit, testable surfaces

Completed work:

- ER-0063 defines graph watch or change-feed APIs over Referee relationships.
- ER-0064 adds relationship-pattern routing in Vizier.

Remaining work:

- add task-state routing in Vizier
- add task view models and Task Conchos
- move Concho creation from helper-driven paths toward observer-driven session growth

ER slices:

- ER-0063 - Referee graph watch and change-feed API - Verified
- ER-0064 - Vizier relationship-pattern routing - Verified
- ER-0065 - task visualization objects and Task Conchos - Proposed
- ER-0066 - observer-driven Conch session growth - Proposed

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

Proposed ER slices:

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

Proposed ER slices:

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

Proposed ER slices:

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

1. Implement ER-0065 task visualization objects and Task Conchos under AR-0025.
2. Recheck ER-0066 scope, then implement observer-driven Conch session growth under AR-0025.
3. Implement ER-0079 and ER-0080 to close the remaining AR-0023 reflection-profile metadata.
4. Add reusable parser surfaces for AR-0026 Level 4.
5. Implement Machine and Comms delivery tracks under AR-0024.
6. Complete the Caliper catalog and runtime conversion work under AR-0019.

## Validation

- `grep -RIn "Status: Proposed" docs/AR`
- `grep -nE "ER-0058|ER-0064|ER-0065|ER-0066|ER-0078|ER-0079|ER-0080" docs/ER/ER-Status.md`
- review this plan against the source plans and [Implementation-Plan-2026-03-14.md](/home/justgus/Dev/irisOS/docs/Plans/Implementation-Plan-2026-03-14.md)
