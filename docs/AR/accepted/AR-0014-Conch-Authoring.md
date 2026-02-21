---
GitHub-Issue: N/A
---

# AR-0014 — Conch Schema and Object Authoring

- Status: Proposed
- Date: 2026-02-19
- Owners: Mike

## Context

Conch can list and inspect existing objects, but it cannot define new schemas or instantiate
objects. This blocks early system bring-up because users cannot create types or persistent objects
without writing C++ or external tooling.

## Decision

Add Conch-native commands for schema authoring and object instantiation that integrate with
Refract and Referee. The shell becomes the first-class bootstrap path for defining types and
creating persistent objects.

### Scope

- Define types (name, namespace, fields, and optional operations) from Conch.
- Create object instances from Conch using a type name and structured field values.
- Persist objects into Referee with DefinitionID/TypeID resolved from Refract.
- Provide a discoverable list of known types.

### Non-Goals

- Complex schema migrations or versioning workflows.
- Full scripting language in Conch.
- Full validation of user-provided payloads beyond basic structural checks.

## Rationale

This enables bootstrapping without requiring external tools and makes Conch the canonical entry
point for evolving the object graph. It also aligns with the system model where the shell operates
on types and objects, not files or programs.

## Consequences

- Conch becomes responsible for some schema validation and persistence.
- Refract and Referee need stable APIs for schema registration and instance creation.
- Future work must define safety/permissions for who can define types.
