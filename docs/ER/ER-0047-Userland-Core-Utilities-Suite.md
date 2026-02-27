---
GitHub-Issue: #TBD
---

# ER-0047 — Userland Core Utilities Suite

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0047
- Title: Userland Core Utilities Suite
- Status: Proposed
- Date: 2026-02-27
- Owners: Mike
- Type: Epic

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: v2 requires a baseline set of object-aware utilities analogous to GNU coreutils.
- Background / constraints: utilities must operate on Referee objects and Refract schemas, not raw files.

## Goals

- Define a minimal set of object-native utilities for listing, inspecting, filtering, and transforming objects.
- Provide consistent output formats for scripting and composability.

## Non-Goals

- Full parity with GNU coreutils feature set.
- Shell scripting language features beyond Conch.

## Scope

- In scope: utilities for list/show/find/edges/ps equivalent operations and batch transforms.
- Out of scope: build tools, compiler toolchains, or package managers.

## Requirements

- Functional: utilities can operate on objects, relationships, and schemas.
- Non-functional: deterministic output and stable flags.

## Proposed Approach

- Summary: implement a small suite of object-aware utilities as Conch commands or standalone tools.
- Alternatives considered: file-oriented tools (rejected for object-native design).

## Acceptance Criteria

- Users can list, inspect, and filter object graphs using utilities.
- Utilities can emit structured output for piping to other tools.

## Risks / Open Questions

- Risk: scope creep toward full GNU parity.
- Question: which minimal commands are required for a usable v2?

## Dependencies

- Dependency 1: ER-0021 Conch Schema and Object Authoring.
- Dependency 2: ER-0028 Conduit Operation Model.

## Implementation Notes

- Notes for implementer: keep command surfaces minimal and consistent.

## Verification Plan

- Tests to run: utility command tests over sample object graphs.
- Manual checks: demonstrate a multi-step object filter pipeline.
