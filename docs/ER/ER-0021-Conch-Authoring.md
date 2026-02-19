# ER-0021 — Conch Schema and Object Authoring

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0021
- Title: Conch Schema and Object Authoring
- Status: Draft
- Date: 2026-02-19
- Owners: Mike
- Type: Enhancement

## Context

Conch can list and inspect existing objects, but it cannot define new schemas or instantiate
objects. This blocks early system bring-up because users cannot create types or persistent objects
without external tooling.

## Goals

- Define types from Conch with names, namespaces, and fields.
- Instantiate objects for a given type from Conch.
- Persist new objects into Referee via Refract-resolved type definitions.
- Provide a discoverable list of available types.

## Non-Goals

- Full schema migration/versioning workflows.
- Complex validation beyond basic structural checks.
- Scripting language features in Conch.

## Scope

- In scope:
  - Conch commands for defining schema and creating instances.
  - Refract/Referee integration for persistence.
- Out of scope:
  - Schema editing UI or migration tooling.

## Requirements

- Functional:
  - Types can be created, listed, and inspected from Conch.
  - Objects can be instantiated and persisted from Conch.
- Non-functional:
  - Deterministic behavior.
  - Minimal diff to existing Conch command surface.

## Proposed Approach

- Add Conch commands for schema definition and object creation using structured arguments.
- Use Refract registry to resolve TypeID/DefinitionID and Referee to store objects.

## Acceptance Criteria

- Users can define a type and create a persistent instance from Conch without external tools.
- Types created through Conch are queryable via existing `ls`/`find type` commands.

## Risks / Open Questions

- Permissions model for authoring schemas.
- Minimal validation vs. strict schema enforcement.

## Dependencies

- ER-0002 Phase 1 Milestone 1
- ER-0003 Phase 1 Milestone 2

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Define a type and instantiate an object from Conch.
