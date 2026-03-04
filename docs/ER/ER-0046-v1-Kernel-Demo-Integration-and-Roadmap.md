---
GitHub-Issue: #182
---

# ER-0046 — v1 Kernel Demo Integration and Roadmap

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0046
- Title: v1 Kernel Demo Integration and Roadmap
- Status: In Progress
- Date: 2026-02-27
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v1 requires a kernel-grade end-to-end demo and a documented v2 roadmap.
- Background / constraints: must exercise persistence, structured types, Conduit ops, and CEO.

## Goals

- Define and execute a v1 kernel demo that validates persistence and operations.
- Document v1 limitations and the v2 roadmap.

## Non-Goals

- Full userland utilities suite.
- GUI compositor or full hardware driver stack.

## Scope

- In scope: demo scenario, integration steps, and roadmap documentation.
- Out of scope: v2 utility implementations.

## Requirements

- Functional: demo exercises structured types, persistence, and operation dispatch.
- Non-functional: deterministic demo steps and outputs.

## Proposed Approach

- Summary: create a guided demo script using Conch + CEO + Referee and document v2 roadmap.
- Alternatives considered: defer integration until all v1 work is complete (rejected).

## Acceptance Criteria

- Demo runs end-to-end with persistent structured data.
- v2 roadmap is published and aligned with implementation plans.

## Risks / Open Questions

- Risk: integration reveals gaps in earlier phases.
- Question: which demo scenario best represents kernel-grade usage?

## Dependencies

- Dependency 1: ER-0034 Referee Storage Layout Implementation (v1).
- Dependency 2: ER-0038 Structured Types: Struct/Packet/Enum.
- Dependency 3: ER-0042 Core Operations Metadata and Bindings.

## Demo Scenario (v1)

### Scenario Summary

The v1 kernel demo exercises:

- persistence of structured types (Demo::PropulsionSynth, Demo::Summary, Demo::Detail)
- operation dispatch (start + expand)
- CEO task hooks (start creates a task record in Conch, demo uses start/stop lifecycle)
- Viz artifact production (TextLog/Metric/Table)
- inter-task comms open/close via CEO primitives

### Demo Steps (Conch)

1. Launch Conch.
2. Create the demo object:
   - `let demo = new Demo::PropulsionSynth name:=PropulsionSynth`
3. Start the demo (creates summary + artifacts):
   - `call demo start`
4. Expand the summary (creates detail records + artifacts):
   - `call <summary_id> expand 2`
5. Inspect output:
   - `edges demo`
   - `show <summary_id>`
   - `route <summary_id>`
6. Exit Conch, relaunch, and re-run `show <summary_id>` to verify persistence.

## v2 Roadmap (Draft)

- Capability enforcement: replace Conch demo capability stub with real capability context.
- Task supervision: integrate CEO task graph with persistent task objects and lifecycle telemetry.
- IO primitives: expand comms integration to cover explicit open/close ownership and auditing.
- Demo evolution: expand the kernel demo to include object graph packaging and import/export.

## Implementation Notes

- Notes for implementer: keep demo script minimal and repeatable.

## Verification Plan

- Tests to run: integration smoke tests.
- Manual checks: follow demo steps and verify outputs.
