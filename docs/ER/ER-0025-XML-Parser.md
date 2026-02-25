---
GitHub-Issue: #106
---


# ER-0025 — XML Parser

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark an ER as Verified.

## ER Metadata

- ER ID: ER-0025
- Title: XML Parser
- Status: Verified
- Date: 2026-02-24
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.
## Context

- Problem statement: IrisOS may need XML parsing for legacy interop or structured data.
- Background / constraints: Keep scope limited to basic XML parsing (no DTD/XSD in v1).

## Goals

- Provide a basic XML parser with element/attribute structure.
- Use the shared Parser Core Framework for tokenization/parsing.

## Non-Goals

- XML validation (DTD/XSD).
- Full namespace and schema resolution in v1.

## Scope

- In scope: XML elements, attributes, text nodes, basic entities.
- Out of scope: validation, XPath/XSLT.

## Requirements

- Functional: parse well-formed XML documents.
- Non-functional: clear error reporting with location.

## Proposed Approach

- Summary: implement a minimal XML grammar on the core parser, with a structured node tree.
- Alternatives considered: adopting a third-party XML library (defer for now).

## Acceptance Criteria

- XML parser handles nested elements and attributes correctly.
- Errors report the location of malformed input.

## Risks / Open Questions

- Risk: XML edge cases increase scope unexpectedly.
- Question: what subset of XML is required for IrisOS use cases?

## Dependencies

- Dependency 1: ER-0022 Parser Core Framework.

## Implementation Notes

- Notes for implementer: keep a strict, small subset for v1.

## Verification Plan

- Tests to run: XML parsing unit tests for nested tags and attributes.
- Manual checks: parse a small XML sample and inspect the node tree.
