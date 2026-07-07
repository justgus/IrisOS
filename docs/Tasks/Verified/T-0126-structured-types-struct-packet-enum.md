---
Legacy-ID: ER-0038
GitHub-Issue: #174
Source-Path: docs/ER/ER-0038-Structured-Types-Struct-Packet-Enum.md
---

# T-0126 — Structured Types: Struct, Packet, Enum

## Task Metadata

- Task ID: T-0126
- Legacy ID: ER-0038
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #174
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0038-Structured-Types-Struct-Packet-Enum.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #174
---

# ER-0038 — Structured Types: Struct, Packet, Enum

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0038
- Title: Structured Types: Struct, Packet, Enum
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: AR-0021 defines structured types (Struct/Packet/Enum) that must be first-class in Refract.
- Background / constraints: Must integrate with Refract Definitions, Referee persistence, and Conch introspection.

## Goals

- Implement Refract schemas for Struct, Packet, and Enum definitions.
- Define deterministic serialization and validation rules for structured types.

## Non-Goals

- Advanced binary layout optimization.
- Full schema migration tooling.

## Scope

- In scope: type definitions, validation, and serialization metadata for structured types.
- Out of scope: compiler-grade layout optimizations or ABI guarantees.

## Requirements

- Functional: Struct/Packet/Enum types can be created, stored, and introspected.
- Non-functional: deterministic layout/ordering rules for packets and enums.

## Proposed Approach

- Summary: extend Refract type kinds and Definition fields to support structured type metadata.
- Alternatives considered: modeling structs as generic Maps (rejected).

## Acceptance Criteria

- Structured types appear in `show type` and include field/tag metadata.
- Packet definitions include ordered bit sequence metadata.

## Risks / Open Questions

- Risk: packet layout rules may need architecture-specific constraints.
- Question: which enum value types are required for v1?

## Dependencies

- Dependency 1: AR-0021 Structured Types, Enums, and Packets.
- Dependency 2: ER-0037 Refract-Native Schema Registry Migration.

## Implementation Notes

- Notes for implementer: keep ordering and layout rules explicit in schema metadata.

## Verification Plan

- Tests to run: create structured types and validate schema persistence.
- Manual checks: define a packet and verify serialized layout metadata.
```
