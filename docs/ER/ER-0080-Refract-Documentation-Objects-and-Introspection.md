---
GitHub-Issue: #288
---

# ER-0080 — Refract Documentation Objects and Introspection

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0080
- Title: Refract Documentation Objects and Introspection
- Status: Proposed
- Date: 2026-03-19
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: Refract can expose structural metadata, but it still lacks authoritative persisted documentation objects or richer descriptive metadata for types, fields, relationships, and operations.
- Background / constraints: AR-0023 lists documentation objects and examples as remaining extended reflection work. Conch introspection can show names and signatures, but it cannot yet rely on Refract for canonical human-facing documentation content.

## Goals

- Introduce persisted documentation metadata or objects linked to Refract definitions.
- Improve introspection surfaces so canonical documentation can be queried alongside structural metadata.
- Keep documentation persistence deterministic and version-aware.

## Non-Goals

- A full external documentation publishing pipeline.
- Rich markup rendering or large narrative manuals in v1.
- Broad policy or validation semantics unrelated to documentation.

## Scope

- In scope: documentation metadata or object model for types, fields, relationships, and operations.
- In scope: persistence, migration, and introspection support for the documentation model.
- In scope: examples or brief canonical descriptions where the model supports them.
- Out of scope: constraint enforcement, effect semantics, and non-Refract documentation systems.

## Requirements

- Functional: relevant Refract entities can carry persisted documentation metadata.
- Functional: introspection surfaces can retrieve canonical descriptions and examples where present.
- Functional: documentation metadata survives persistence round-trips and schema evolution.
- Non-functional: documentation storage remains deterministic and does not break existing bootstrap flows.

## Proposed Approach

- Summary: add a minimal but explicit documentation model to Refract, persist it with the schema graph, and extend introspection surfaces so documentation becomes part of the authoritative reflection view rather than external prose only.
- Alternatives considered: leaving documentation solely in markdown or code comments was rejected because it keeps user-facing introspection disconnected from the reflection system of record.

## Acceptance Criteria

- Schema round-trip tests verify persisted documentation metadata or linked documentation objects.
- Introspection surfaces expose documentation content for types or operations that declare it.
- Existing definitions without documentation continue to load and display normally.

## Risks / Open Questions

- Risk: storing too much free-form content directly in schema definitions could complicate migration and display rules.
- Question: should examples be embedded directly on reflected entities in this ER or represented as linked documentation objects from the start?

## Dependencies

- Dependency 1: AR-0023 Refract Reflection Profiles.
- Dependency 2: ER-0037 Refract-Native Schema Registry Migration.
- Dependency 3: ER-0055 Refract Inheritance and Interface Metadata.

## Implementation Notes

- Notes for implementer: prefer a small documentation model that fits existing introspection output and can grow later without rewriting stored schema records.

## Verification Plan

- Tests to run:
  - `make check`
- Manual checks:
  - inspect a type or operation with documentation metadata through Conch.
  - confirm older definitions without documentation still load without migration surprises.
