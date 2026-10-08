---
GitHub-Issue: N/A
AR-Dependencies: AR-0011
ER-Dependencies: ER-0009, ER-0064
---

# ER-0087 — Vizier Operation and Emitted-Artifact Metadata

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: In this Autonomous AR Batch, Codex may verify only after all batch gates pass.

## ER Metadata

- ER ID: ER-0087
- Title: Vizier Operation and Emitted-Artifact Metadata
- Status: In Progress
- Date: 2026-10-07
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0011 assigns Vizier responsibility for using Refract operation and emitted-artifact metadata. Refract stores operation effects, including `Emits`, but current Vizier routing selects from realized artifact type and preferred-renderer metadata without consulting the producing operation metadata.
- Background / constraints: Existing ER-0009 and ER-0064 routing must remain deterministic. Operation metadata informs interpretation; it does not authorize execution or replace the realized artifact type as the source of truth for an existing object.

## Goals

- Expose deterministic Vizier interpretation of an operation's emitted-artifact metadata.
- Let callers inspect operation-to-artifact route hints without executing the operation or creating speculative Conchos.
- Preserve current routing for artifacts that already exist in the graph.

## Non-Goals

- Execute operations, grant capabilities, or change CEO scheduling.
- Spawn Conchos for artifacts that have not actually been created.
- Change operation metadata schemas or introduce a new renderer policy.

## Scope

- In scope: a Vizier query that reads a type's operation definitions and `Emits` effects, resolves declared target artifact types to existing preferred-renderer/type routes, returns stable operation/artifact/route descriptors, and focused tests.
- Out of scope: operation execution, metadata authoring changes, graph relationship routing changes, and rendering.

## Requirements

- Functional: an `Emits` target denotes a Refract type only when it exactly matches the canonical qualified name `namespace::type` of a registered type. For such targets, Vizier returns a descriptor containing the operation name, declared artifact type, and route selected by the same type/preferred-renderer rules used for realized artifacts.
- Functional: operations without `Emits` effects do not produce emitted-artifact descriptors.
- Functional: an `Emits` target that is not a canonical registered type name is preserved as an opaque target in an unroutable descriptor; a registered type with no route is also returned as unroutable. Tests cover both cases.
- Functional: metadata inspection does not execute the operation or create objects or Conchos.
- Non-functional: existing `route_for_type`, `route_for_relationship`, and session-growth results remain unchanged.

## Proposed Approach

- Summary: add a read-only Vizier interpretation API over Refract operation definitions and their `Emits` effects. Resolve only canonical `namespace::type` targets through the existing type registry and routing logic; preserve every other target string as opaque metadata. Preserve declaration order. Add tests for registered type targets, opaque names such as `demo.events`, registered but unroutable types, preferred renderers, and non-Emits effects.
- Alternatives considered: routing only from realized graph edges was rejected as insufficient to satisfy the AR's explicit operation-metadata responsibility; spawning views from declarations alone was rejected because metadata describes possible effects, not completed object creation.

## Acceptance Criteria

- Tests verify operation names, emitted type targets, and routes are returned in stable declaration order.
- Tests verify preferred-renderer metadata is honored for declared artifact types.
- Tests verify non-Emits effects and operations without emitted targets do not create emitted-artifact descriptors.
- Tests verify canonical type targets resolve, opaque non-type targets such as `demo.events` remain unroutable descriptors, and neither case throws or mutates Referee state.
- Existing `test_vizier_routing` and `test_refract_registry` pass.

## Risks / Open Questions

- Risk: existing `OperationEffect::target` strings may not consistently name Refract types; the canonical qualified-name rule preserves non-type values without misclassifying them.
- Question: none; the target interpretation and unresolved-name behavior are defined above.

## Dependencies

- Dependency 1: ER-0009 Phase 3 Vizier Routing.
- Dependency 2: ER-0064 Vizier Relationship-Pattern Routing.
- Dependency 3: AR-0011 Vizier Interpretation Layer.

## Implementation Notes

- Reuse existing type lookup and route selection rather than maintaining a second route table.
- Preserve declared operation effect order.
- Do not mutate the graph while generating route descriptors.

## Verification Plan

- Tests to run:
  - `make -j`
  - `make -C tests test_vizier_routing test_refract_registry`
  - `make check`
- Manual checks: none; this read-only interpretation API is fully observable through deterministic tests.

### Live Pass (Human)

- Required: No
- If no, reason: The API returns deterministic metadata descriptors and does not change visible shell or device behavior.
- Build / environment / target: N/A
- Steps: N/A
- Expected observations: N/A
- Observed results: N/A
- Result: Not Required
- Performed by: N/A
- Date: N/A
