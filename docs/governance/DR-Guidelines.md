---
GitHub-Issue: N/A
---

# DR Guidelines

## Purpose

Discrepancy Reports (DRs) capture defects, regressions, behavioral mismatches, and other cases
where the system does not meet expected behavior.

Use a DR when you need a durable record of:

- what is broken
- how to reproduce it
- user and system impact
- severity and priority
- how the fix will be verified

Do not use a DR for:

- architecture decisions
- feature requests without an observed discrepancy

## Document Shape

DRs should follow `docs/DR/DR-Template.md`. The most important fields are:

- summary
- environment
- steps to reproduce
- expected behavior
- actual behavior
- impact
- evidence
- triage notes
- fix plan
- verification plan

A DR should make reproduction and validation straightforward for someone who was not present when
the issue was first reported.

## Severity and Priority

Use severity for technical/user impact and priority for scheduling urgency.

- Severity:
  - `Critical`: data loss, corruption, or core system unusable
  - `High`: major feature broken or strong workflow regression
  - `Medium`: clear defect with viable workaround
  - `Low`: limited impact or cosmetic/ergonomic issue
- Priority:
  - `P0`: immediate attention
  - `P1`: next practical fix window
  - `P2`: important but not urgent
  - `P3`: backlog candidate

Do not use priority as a substitute for severity; record both.

## Status Lifecycle

DR documents may use:

- `New`
- `Triaged`
- `Proposed`
- `In Progress`
- `Fixed`
- `Complete`
- `Verified`
- `Deferred`
- `Rejected`

Recommended usage:

- `New`: reported, not yet assessed
- `Triaged`: reproduced or analyzed enough to scope impact
- `Proposed`: fix direction chosen
- `In Progress`: fix underway
- `Fixed` / `Complete`: change landed, awaiting verification
- `Verified`: system engineer has confirmed the fix
- `Deferred`: real issue, intentionally postponed
- `Rejected`: not a bug or not accepted for action

Only the System Engineer marks a DR as `Verified`.

## Relationship to ERs

Use a DR as the problem record and an ER as the implementation plan when the fix is substantial.

- Small, direct fixes may be handled directly from the DR.
- Larger fixes should reference an ER in the DR triage or fix plan.
- Keep the DR focused on the discrepancy even if the implementation spans multiple commits.

## GitHub Tracking

- Use label `dr` for the issue type.
- Use one coarse-grained status label on the issue.

Issue label mapping is broader than document status:

- `status:proposed` maps to `New`, `Triaged`, and `Proposed`
- `status:in-progress` maps to `In Progress`
- `status:done` maps to `Fixed`, `Complete`, `Verified`, `Deferred`, and `Rejected`

The DR document is the authoritative status record when more precision is needed.

## Review Standard

A DR is ready for implementation when:

- reproduction is concrete or the failure evidence is strong
- impact is explicit
- the suspected cause is narrow enough to act on
- verification steps prove the issue is fixed rather than merely hidden
