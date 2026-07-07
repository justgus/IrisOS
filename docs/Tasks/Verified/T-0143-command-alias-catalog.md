---
Legacy-ID: ER-0047.4
GitHub-Issue: #182
Source-Path: docs/ER/ER-0047.4-Command-Alias-Catalog.md
---

# T-0143 — Command Alias Catalog

## Task Metadata

- Task ID: T-0143
- Legacy ID: ER-0047.4
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #182
- AR Dependencies: -
- Date Requested: 2026-03-07
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0047.4-Command-Alias-Catalog.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #182
---

# ER-0047.4 — Command Alias Catalog

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0047.4
- Title: Command Alias Catalog
- Status: Verified
- Date: 2026-03-07
- Owners: Mike
- Type: Enhancement

## Context

- Alias bindings are required to map CLI command names to session operations.

## Goals

- Define and document a stable alias catalog for Conch commands.
- Ensure aliases are discoverable and consistent with operation names.

## Non-Goals

- Changing operation behavior or adding new utilities.

## Scope

- In scope: alias definitions for all Conch commands.
- In scope: documentation of alias-to-operation mappings.

## Implementation Notes

- Alias catalog is exposed via the Conch `aliases` command (session operation `aliases_list`).

## Requirements

- Functional: aliases resolve to session operations without ambiguity.
- Non-functional: alias catalog is deterministic and versioned.

## Proposed Approach

- Store alias mappings in schema bootstrap or a dedicated Conch catalog.
- Expose alias list through Conch (e.g., `ops` or dedicated command).

## Acceptance Criteria

- All commands have an alias mapping.
- Alias list can be queried and tested.

## Dependencies

- ER-0047.1 Conch Session Operation Model.

## Verification Plan

- Tests: alias resolution tests for all commands.
```
