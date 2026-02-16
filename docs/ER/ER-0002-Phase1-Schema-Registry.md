# ER-0002 — Phase 1 Milestone 1: Refract Schema Registry v0

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0002
- Title: Phase 1 Milestone 1 — Refract Schema Registry v0
- Status: Draft
- Date: 2026-02-16
- Owners: Mike
- Type: Enhancement

## Context

Phase 1 begins with a minimal Refract schema registry so objects can be defined, validated, and
introspected by Conch/CEO.

## Goals

- Define minimal Refract schema objects (Type, Class, Definition, FieldDefinition,
  OperationDefinition, Signature, RelationshipSpec).
- Store schemas in Referee and enable lookup by TypeID/DefinitionID.
- Provide a minimal registry API for Conch and CEO.

## Non-Goals

- Full schema migration tooling.
- Advanced constraints or policy enforcement.

## Scope

- In scope:
  - Refract object definitions (C++ types + serialization).
  - Registry CRUD operations for schemas.
  - Lookup by TypeID and DefinitionID.
- Out of scope:
  - Full editor tooling for schema authoring.

## Requirements

- Functional:
  - Create and persist schema objects in Referee.
  - Retrieve Definition + related Field/Operation entries.
  - Enumerate types for Conch discovery.
- Non-functional:
  - Deterministic storage.
  - C++20/24 baseline.

## Proposed Approach

- Define Refract schema objects as C++ types (initially in Erector::Refract).
- Serialize schema objects to CBOR and store in Referee.
- Implement a minimal registry service:
  - register_definition
  - get_definition
  - list_types

## Acceptance Criteria

- Schemas can be created and persisted in Referee.
- Definition retrieval returns full field/operation lists.
- Conch can list types via the registry.

## Risks / Open Questions

- Need a stable TypeID policy early to avoid churn.

## Dependencies

- AR-0007 Refract Reflection Graph
- AR-0009 Referee Object Store

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - Create a schema and verify lookup in Conch shell (later phase).
