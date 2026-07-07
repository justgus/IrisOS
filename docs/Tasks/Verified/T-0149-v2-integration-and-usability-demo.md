---
Legacy-ID: ER-0053
GitHub-Issue: #189
Source-Path: docs/ER/ER-0053-v2-Integration-and-Usability-Demo.md
---

# T-0149 — v2 Integration and Usability Demo

## Task Metadata

- Task ID: T-0149
- Legacy ID: ER-0053
- Status: Verified
- Source Status: Verified
- Epic: EP-001
- Sprint Assigned: N/A - migrated historical record
- GitHub Issue: #189
- AR Dependencies: -
- Date Requested: 2026-02-27
- Date Migrated: 2026-07-07
- Owners: Mike
- Source Path: docs/ER/ER-0053-v2-Integration-and-Usability-Demo.md

## Migration Notes

This Airframe Task was generated from the legacy ER record during the local documentation migration. The target status represents the approved source documentation state and is not a new verification action.

## Preserved Legacy ER Content

```md
---
GitHub-Issue: #189
---

# ER-0053 — v2 Integration and Usability Demo

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0053
- Title: v2 Integration and Usability Demo
- Status: Verified
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v2 requires an end-to-end userland workflow that proves utility usefulness.
- Background / constraints: must combine parsers, utilities, and Conch into a cohesive demo.

## Goals

- Demonstrate a userland workflow using v2 utilities and parsers.
- Document v2 limitations and draft v3 roadmap.

## Non-Goals

- Full production-ready userland suite.
- GUI desktop environment.

## Scope

- In scope: demo workflow and documentation.
- Out of scope: new kernel features.

## Requirements

- Functional: demo uses parsers, utilities, and Conch end-to-end.
- Non-functional: deterministic steps and repeatable outputs.

## Proposed Approach

- Summary: create a demo script that imports data, transforms it, and publishes results.
- Alternatives considered: skip demo and rely on unit tests only (rejected).

## Acceptance Criteria

- Demo runs using v2 utilities and produces expected outputs.
- v3 roadmap is documented.

## Risks / Open Questions

- Risk: demo reveals missing utility coverage.
- Question: which workflow best represents v2 value?

## Dependencies

- Dependency 1: ER-0047 Userland Core Utilities Suite.
- Dependency 2: ER-0048 Referee Import/Export Tools.
- Dependency 3: ER-0050 Schema Migration Tools.

## Implementation Notes

- Notes for implementer: keep the demo minimal and scripted.
- Implementation:
  - Added `scripts/demo_v2.sh` to run a repeatable two-stage v2 workflow over fresh Referee stores.
  - Producer stage exercises `define type --json`, `new --json`, `demo v1`, task profiling/tracing, and `bundle export`.
  - Consumer stage exercises `bundle import`, `show`, `debug graph`, `debug dispatch`, and imported type inspection.
  - Added an automated smoke test in `tests/test_conch_authoring.cc` that runs the demo script and checks for stable output markers.

## Demo Scenario (v2)

### Scenario Summary

The v2 demo exercises:

- JSON-driven type definition and object creation through Conch parsers
- userland workflow composition across shell commands and object utilities
- task profiling and trace export into Viz artifacts
- bundle export/import across isolated Referee stores
- imported object inspection and debug tracing in a fresh store

### Demo Steps

1. Build `bin/conch`.
2. Run `bash scripts/demo_v2.sh`.
3. Observe the producer stage:
   - define `DemoV2::Widget`
   - create a widget via `new --json`
   - run `demo v1`
   - capture task profile/trace artifacts
   - export a bundle
4. Observe the consumer stage:
   - import the bundle into a fresh store
   - inspect imported objects
   - run `debug graph`
   - run `debug dispatch`
5. Confirm the final `DEMO_V2_OK` marker.

## v3 Roadmap (Draft)

- Add first-class script execution support to Conch so demos do not rely on stdin redirection.
- Expose versioned schema authoring flows in Conch for migration demos without test-only setup helpers.
- Expand parser-backed ingestion beyond JSON so XML/C++/Python artifacts can participate in the same workflow.
- Add richer bundle selection/filtering so demos can export/import targeted object subgraphs instead of whole-store snapshots.
- Promote debug and profile outputs into a higher-level scripted report command for usability testing.

## Verification Plan

- Tests to run: integration smoke tests.
- Manual checks: execute the demo steps and verify outputs.
```
