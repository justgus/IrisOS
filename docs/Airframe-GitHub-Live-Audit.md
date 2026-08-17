# Airframe GitHub Live Audit

**Date:** 2026-08-17
**Scope:** Read-only GitHub comparison
**Epic:** EP-001
**Task:** T-0081

## Boundary

This audit compares the checked-in AR, ER, and DR inventory with live GitHub issue metadata. It does not create, edit, label, close, or reopen GitHub issues. Local documentation remains authoritative until the System Engineer approves a correction or synchronization operation.

## Summary

| Check | Result |
| ----- | ------ |
| Local records with numeric GitHub references | 111 |
| Unique referenced GitHub issues | 102 |
| Referenced issues found | 102 |
| Missing referenced issues | 0 |
| Type-label mismatches | 0 |
| Title mismatches after accepting the standard record-ID prefix | 1 (`#116`) |
| Status-label mismatches | 46 unique issues |
| State mismatches for Verified ER/DR records | 2 (`#287`, `#288`) |
| Shared references retained for review | `#181`, `#182`, `#184` |
| AR records without numeric references | AR-0018, AR-0019, AR-0020, AR-0021 |

## Actionable Drift

| GitHub Issue | Local Record(s) | Local Status | Live State | Live Status Label | Diagnosis |
| ------------ | --------------- | ------------ | ---------- | ----------------- | --------- |
| #116 | DR-0001 | Verified | Closed | `status:in-progress` | Generic template title and stale status label; expected DR record title and `status:done`. |
| #287 | ER-0079 | Verified | Open | `status:proposed` | State and status label drift; expected Closed and `status:done`. |
| #288 | ER-0080 | Verified | Open | `status:proposed` | State and status label drift; expected Closed and `status:done`. |

In addition, 43 closed issues have stale status labels relative to local source status: 12 Implemented AR issues and 31 Verified ER issues. They are enumerated in the snapshot below. The five Accepted AR issues `#290` through `#294` are open with `status:accepted`; that is consistent with the repository policy and is not classified as drift.

## Shared Reference Review

| GitHub Issue | Local Records | Live Title | Finding |
| ------------ | ------------- | ---------- | ------- |
| #181 | ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4 | ER-0045 — Kernel I/O Primitives Integration | Parent record controls the shared issue title. |
| #182 | ER-0046, ER-0047.1, ER-0047.2, ER-0047.3, ER-0047.4 | ER-0046 — v1 Kernel Demo Integration and Roadmap | Parent record controls the shared issue title. |
| #184 | ER-0048, ER-0054 | ER-0054 — Conch Namespace Navigation | ER-0054 controls the title; ER-0048 is not represented by the live title. |

No mapping was normalized in this audit.

## Live Snapshot

| Issue | Local Record(s) | Local Status | Live State | Live Title | Live Labels | Finding |
| ----- | --------------- | ------------ | ---------- | ---------- | ----------- | ------- |
| [#65](https://github.com/justgus/IrisOS/issues/65) | AR-0001 | Accepted | Closed | AR-0001 — Expected-Style Error Model | `ar`, `status:accepted` | Aligned |
| [#66](https://github.com/justgus/IrisOS/issues/66) | AR-0002 | Implemented | Closed | AR-0002 — Object Identity and Versioning Semantics | `ar`, `status:accepted` | Expected `status:done` |
| [#67](https://github.com/justgus/IrisOS/issues/67) | AR-0003 | Implemented | Closed | AR-0003 — Storage Layout Strategy | `ar`, `status:accepted` | Expected `status:done` |
| [#68](https://github.com/justgus/IrisOS/issues/68) | AR-0004 | Implemented | Closed | AR-0004 — Index and Graph Storage Strategy (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#69](https://github.com/justgus/IrisOS/issues/69) | AR-0005 | Implemented | Closed | AR-0005 — Service Plane Model (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#70](https://github.com/justgus/IrisOS/issues/70) | AR-0006 | Implemented | Closed | AR-0006 — CEO/Exec Runtime Model (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#71](https://github.com/justgus/IrisOS/issues/71) | AR-0007 | Implemented | Closed | AR-0007 — Refract Reflection Graph (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#72](https://github.com/justgus/IrisOS/issues/72) | AR-0008 | Implemented | Closed | AR-0008 — Erector Subsystems (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#73](https://github.com/justgus/IrisOS/issues/73) | AR-0009 | Implemented | Closed | AR-0009 — Referee Object Store (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#74](https://github.com/justgus/IrisOS/issues/74) | AR-0010 | Implemented | Closed | AR-0010 — Comms Subsystem (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#75](https://github.com/justgus/IrisOS/issues/75) | AR-0011 | Accepted | Closed | AR-0011 — Vizier Interpretation Layer (Recommendation) | `ar`, `status:accepted` | Aligned |
| [#76](https://github.com/justgus/IrisOS/issues/76) | AR-0012 | Accepted | Closed | AR-0012 — Conch Shell and Concho Views (Recommendation) | `ar`, `status:accepted` | Aligned |
| [#77](https://github.com/justgus/IrisOS/issues/77) | AR-0013 | Implemented | Closed | AR-0013 — Viz Display Subsystem (Recommendation) | `ar`, `status:accepted` | Expected `status:done` |
| [#78](https://github.com/justgus/IrisOS/issues/78) | AR-0014 | Implemented | Closed | AR-0014 — Conch Schema and Object Authoring | `ar`, `status:proposed` | Expected `status:done` |
| [#79](https://github.com/justgus/IrisOS/issues/79) | AR-0015 | Accepted | Closed | AR-0015 — Conch Parser and Syntax | `ar`, `status:accepted` | Aligned |
| [#80](https://github.com/justgus/IrisOS/issues/80) | AR-0016 | Implemented | Closed | AR-0016 — Operations and Dispatch Model | `ar`, `status:accepted` | Expected `status:done` |
| [#81](https://github.com/justgus/IrisOS/issues/81) | AR-0017 | Accepted | Closed | AR-0017 — Collections, Math Types, and Units | `ar`, `status:accepted` | Aligned |
| [#82](https://github.com/justgus/IrisOS/issues/82) | ER-0001 | Verified | Closed | ER-0001 — Service Model and IPC Foundation | `er`, `status:done` | Aligned |
| [#83](https://github.com/justgus/IrisOS/issues/83) | ER-0002 | Verified | Closed | ER-0002 — Phase 1 Milestone 1: Refract Schema Registry v0 | `er`, `status:done` | Aligned |
| [#84](https://github.com/justgus/IrisOS/issues/84) | ER-0003 | Verified | Closed | ER-0003 — Phase 1 Milestone 2: Referee Graph Enhancements v0 | `er`, `status:done` | Aligned |
| [#85](https://github.com/justgus/IrisOS/issues/85) | ER-0004 | Verified | Closed | ER-0004 — Phase 1 Integration: Schema Registry + Referee Graph | `er`, `status:done` | Aligned |
| [#86](https://github.com/justgus/IrisOS/issues/86) | ER-0005 | Verified | Closed | ER-0005 — Phase 2 Milestone 1: CEO Task Lifecycle v0 | `er`, `status:done` | Aligned |
| [#87](https://github.com/justgus/IrisOS/issues/87) | ER-0006 | Verified | Closed | ER-0006 — Phase 2 Milestone 2: Exec Waitables v0 | `er`, `status:done` | Aligned |
| [#88](https://github.com/justgus/IrisOS/issues/88) | ER-0007 | Verified | Closed | ER-0007 — Phase 2 Integration: CEO + Exec Waitables | `er`, `status:done` | Aligned |
| [#89](https://github.com/justgus/IrisOS/issues/89) | ER-0008 | Verified | Closed | ER-0008 — Phase 3 Milestone 1: Conch Shell v0 | `er`, `status:done` | Aligned |
| [#90](https://github.com/justgus/IrisOS/issues/90) | ER-0009 | Verified | Closed | ER-0009 — Phase 3 Milestone 2: Vizier Routing v0 | `er`, `status:done` | Aligned |
| [#91](https://github.com/justgus/IrisOS/issues/91) | ER-0010 | Verified | Closed | ER-0010 — Phase 3 Milestone 3: Viz Artifacts v0 | `er`, `status:done` | Aligned |
| [#92](https://github.com/justgus/IrisOS/issues/92) | ER-0011 | Verified | Closed | ER-0011 — Phase 3 Integration: Conch + Vizier + Viz | `er`, `status:done` | Aligned |
| [#93](https://github.com/justgus/IrisOS/issues/93) | ER-0012 | Verified | Closed | ER-0012 — Phase 4 Milestone 1: Demo Object Core | `er`, `status:proposed` | Expected `status:done` |
| [#94](https://github.com/justgus/IrisOS/issues/94) | ER-0013 | Verified | Closed | ER-0013 — Phase 4 Milestone 2: Composite Summary Pattern | `er`, `status:proposed` | Expected `status:done` |
| [#95](https://github.com/justgus/IrisOS/issues/95) | ER-0014 | Verified | Closed | ER-0014 — Phase 4 Integration: Demo End-to-End | `er`, `status:proposed` | Expected `status:done` |
| [#96](https://github.com/justgus/IrisOS/issues/96) | ER-0015 | Verified | Closed | ER-0015 — Phase 5 Milestone 1: Comms Primitives v0 | `er`, `status:proposed` | Expected `status:done` |
| [#97](https://github.com/justgus/IrisOS/issues/97) | ER-0016 | Verified | Closed | ER-0016 — Phase 5 Milestone 2: CEO I/O Reactor v0 | `er`, `status:proposed` | Expected `status:done` |
| [#98](https://github.com/justgus/IrisOS/issues/98) | ER-0017 | Verified | Closed | ER-0017 — Phase 5 Integration: Comms + Reactor | `er`, `status:proposed` | Expected `status:done` |
| [#99](https://github.com/justgus/IrisOS/issues/99) | ER-0018 | Verified | Closed | ER-0018 — Phase 6 Milestone 1: Storage Alignment | `er`, `status:proposed` | Expected `status:done` |
| [#100](https://github.com/justgus/IrisOS/issues/100) | ER-0019 | Verified | Closed | ER-0019 — Phase 6 Milestone 2: Persistence and Migration Tests | `er`, `status:proposed` | Expected `status:done` |
| [#101](https://github.com/justgus/IrisOS/issues/101) | ER-0020 | Verified | Closed | ER-0020 — Phase 6 Integration: Hardening and Alignment | `er`, `status:proposed` | Expected `status:done` |
| [#102](https://github.com/justgus/IrisOS/issues/102) | ER-0021 | Verified | Closed | ER-0021 — Conch Schema and Object Authoring | `er`, `status:in-progress` | Expected `status:done` |
| [#103](https://github.com/justgus/IrisOS/issues/103) | ER-0022 | Verified | Closed | ER-0022 — Parser Core Framework | `er`, `status:proposed` | Expected `status:done` |
| [#104](https://github.com/justgus/IrisOS/issues/104) | ER-0023 | Verified | Closed | ER-0023 — Conch Parser Implementation | `er`, `status:proposed` | Expected `status:done` |
| [#105](https://github.com/justgus/IrisOS/issues/105) | ER-0024 | Verified | Closed | ER-0024 — JSON Parser | `er`, `status:proposed` | Expected `status:done` |
| [#106](https://github.com/justgus/IrisOS/issues/106) | ER-0025 | Verified | Closed | ER-0025 — XML Parser | `er`, `status:proposed` | Expected `status:done` |
| [#107](https://github.com/justgus/IrisOS/issues/107) | ER-0026 | Verified | Closed | ER-0026 — C++ Parser | `er`, `status:proposed` | Expected `status:done` |
| [#108](https://github.com/justgus/IrisOS/issues/108) | ER-0027 | Verified | Closed | ER-0027 — Python Parser | `er`, `status:proposed` | Expected `status:done` |
| [#109](https://github.com/justgus/IrisOS/issues/109) | ER-0028 | Verified | Closed | ER-0028 — Conduit Operation Model | `er`, `status:proposed` | Expected `status:done` |
| [#110](https://github.com/justgus/IrisOS/issues/110) | ER-0029 | Verified | Closed | ER-0029 — Conduit Dispatch Engine | `er`, `status:proposed` | Expected `status:done` |
| [#111](https://github.com/justgus/IrisOS/issues/111) | ER-0030 | Verified | Closed | ER-0030 — Conduit Conch Integration | `er`, `status:proposed` | Expected `status:done` |
| [#112](https://github.com/justgus/IrisOS/issues/112) | ER-0031 | Verified | Closed | ER-0031 — Crate Collections | `er`, `status:proposed` | Expected `status:done` |
| [#113](https://github.com/justgus/IrisOS/issues/113) | ER-0032 | Verified | Closed | ER-0032 — Astra Math Types | `er`, `status:proposed` | Expected `status:done` |
| [#114](https://github.com/justgus/IrisOS/issues/114) | ER-0033 | Verified | Closed | ER-0033 — Caliper Units and Quantities | `er`, `status:proposed` | Expected `status:done` |
| [#116](https://github.com/justgus/IrisOS/issues/116) | DR-0001 | Verified | Closed | DR Template — Discrepancy Report / Bug / Issue / Task | `dr`, `status:in-progress` | Title drift; Expected `status:done` |
| [#171](https://github.com/justgus/IrisOS/issues/171) | ER-0035 | Verified | Closed | ER-0035 — Referee Recovery and Rebuild Workflows | `er`, `status:done` | Aligned |
| [#172](https://github.com/justgus/IrisOS/issues/172) | ER-0036 | Verified | Closed | ER-0036 — Definition Versioning and Migration Hooks | `er`, `status:done` | Aligned |
| [#173](https://github.com/justgus/IrisOS/issues/173) | ER-0037 | Verified | Closed | ER-0037 — Refract-Native Schema Registry Migration | `er`, `status:done` | Aligned |
| [#174](https://github.com/justgus/IrisOS/issues/174) | ER-0038 | Verified | Closed | ER-0038 — Structured Types: Struct, Packet, Enum | `er`, `status:done` | Aligned |
| [#175](https://github.com/justgus/IrisOS/issues/175) | ER-0039 | Verified | Closed | ER-0039 — Structured Arrays and Collection Integration | `er`, `status:done` | Aligned |
| [#176](https://github.com/justgus/IrisOS/issues/176) | ER-0040 | Verified | Closed | ER-0040 — Generic Type System Core | `er`, `status:done` | Aligned |
| [#177](https://github.com/justgus/IrisOS/issues/177) | ER-0041 | Verified | Closed | ER-0041 — Scoped Type Registry | `er`, `status:done` | Aligned |
| [#178](https://github.com/justgus/IrisOS/issues/178) | ER-0042 | Verified | Closed | ER-0042 — Core Operations Metadata and Bindings | `er`, `status:done` | Aligned |
| [#179](https://github.com/justgus/IrisOS/issues/179) | ER-0043 | Verified | Closed | ER-0043 — CEO Runtime Hardening | `er`, `status:proposed` | Expected `status:done` |
| [#180](https://github.com/justgus/IrisOS/issues/180) | ER-0044 | Verified | Closed | ER-0044 — Capability Hooks and Policy Plumbing | `er`, `status:proposed` | Expected `status:done` |
| [#181](https://github.com/justgus/IrisOS/issues/181) | ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4 | Verified | Closed | ER-0045 — Kernel I/O Primitives Integration | `er`, `status:proposed` | Expected `status:done`; Shared reference |
| [#182](https://github.com/justgus/IrisOS/issues/182) | ER-0046, ER-0047.1, ER-0047.2, ER-0047.3, ER-0047.4 | Verified | Closed | ER-0046 — v1 Kernel Demo Integration and Roadmap | `er`, `status:proposed` | Expected `status:done`; Shared reference |
| [#183](https://github.com/justgus/IrisOS/issues/183) | ER-0047 | Verified | Closed | ER-0047 — Userland Core Utilities Suite | `er`, `status:proposed` | Expected `status:done` |
| [#184](https://github.com/justgus/IrisOS/issues/184) | ER-0048, ER-0054 | Verified | Closed | ER-0054 — Conch Namespace Navigation | `er`, `status:done` | Shared reference |
| [#185](https://github.com/justgus/IrisOS/issues/185) | ER-0049 | Verified | Closed | ER-0049 — Object Graph Bundles and Packages | `er`, `status:proposed` | Expected `status:done` |
| [#186](https://github.com/justgus/IrisOS/issues/186) | ER-0050 | Verified | Closed | ER-0050 — Schema Migration Tools | `er`, `status:proposed` | Expected `status:done` |
| [#187](https://github.com/justgus/IrisOS/issues/187) | ER-0051 | Verified | Closed | ER-0051 — CEO Profiling and Trace Utilities | `er`, `status:proposed` | Expected `status:done` |
| [#188](https://github.com/justgus/IrisOS/issues/188) | ER-0052 | Verified | Closed | ER-0052 — Conch Debug and Inspection Tools | `er`, `status:proposed` | Expected `status:done` |
| [#189](https://github.com/justgus/IrisOS/issues/189) | ER-0053 | Verified | Closed | ER-0053 — v2 Integration and Usability Demo | `er`, `status:done` | Aligned |
| [#190](https://github.com/justgus/IrisOS/issues/190) | ER-0034 | Verified | Closed | ER-0034 — Referee Storage Layout Implementation (v1) | `er`, `status:done` | Aligned |
| [#262](https://github.com/justgus/IrisOS/issues/262) | ER-0055 | Verified | Closed | ER-0055 — Refract Inheritance and Interface Metadata | `er`, `status:done` | Aligned |
| [#263](https://github.com/justgus/IrisOS/issues/263) | ER-0056 | Verified | Closed | ER-0056 — Conch Typed AST and Shell Decomposition | `er`, `status:done` | Aligned |
| [#264](https://github.com/justgus/IrisOS/issues/264) | ER-0057 | Verified | Closed | ER-0057 — Service Host Lifecycle and Persistent Registry | `er`, `status:done` | Aligned |
| [#265](https://github.com/justgus/IrisOS/issues/265) | ER-0058 | Verified | Closed | ER-0058 — Capability Context Objects and Persistence | `er`, `status:done` | Aligned |
| [#266](https://github.com/justgus/IrisOS/issues/266) | ER-0059 | Verified | Closed | ER-0059 — CEO Task Capability Attachment | `er`, `status:done` | Aligned |
| [#267](https://github.com/justgus/IrisOS/issues/267) | ER-0060 | Verified | Closed | ER-0060 — Service Boundary Capability Enforcement | `er`, `status:done` | Aligned |
| [#268](https://github.com/justgus/IrisOS/issues/268) | ER-0061 | Verified | Closed | ER-0061 — Sandbox Identity and Service Isolation Hooks | `er`, `status:done` | Aligned |
| [#269](https://github.com/justgus/IrisOS/issues/269) | ER-0062 | Verified | Closed | ER-0062 — Memory Service Baseline | `er`, `status:done` | Aligned |
| [#270](https://github.com/justgus/IrisOS/issues/270) | ER-0063 | Verified | Closed | ER-0063 — Referee Graph Watch and Change-Feed API | `er`, `status:done` | Aligned |
| [#271](https://github.com/justgus/IrisOS/issues/271) | ER-0064 | Verified | Closed | ER-0064 — Vizier Relationship-Pattern Routing | `er`, `status:done` | Aligned |
| [#272](https://github.com/justgus/IrisOS/issues/272) | ER-0065 | Verified | Closed | ER-0065 — Task Visualization Objects and Task Conchos | `er`, `status:done` | Aligned |
| [#273](https://github.com/justgus/IrisOS/issues/273) | ER-0066 | Verified | Closed | ER-0066 — Observer-Driven Conch Session Growth | `er`, `status:done` | Aligned |
| [#274](https://github.com/justgus/IrisOS/issues/274) | ER-0067 | Verified | Closed | ER-0067 — Reusable Conch Grammar API | `er`, `status:done` | Aligned |
| [#275](https://github.com/justgus/IrisOS/issues/275) | ER-0068 | Verified | Closed | ER-0068 — Batch Execution and Non-Interactive Parser Integration | `er`, `status:done` | Aligned |
| [#276](https://github.com/justgus/IrisOS/issues/276) | ER-0069 | Verified | Closed | ER-0069 — Shared Parser Regression Harness | `er`, `status:done` | Aligned |
| [#278](https://github.com/justgus/IrisOS/issues/278) | ER-0070 | Proposed | Open | ER-0070 — Machine Representation Primitives | `er`, `status:proposed` | Aligned |
| [#279](https://github.com/justgus/IrisOS/issues/279) | ER-0071 | Proposed | Open | ER-0071 — Machine Descriptors and Resource Facts | `er`, `status:proposed` | Aligned |
| [#280](https://github.com/justgus/IrisOS/issues/280) | ER-0072 | Proposed | Open | ER-0072 — Machine Handles and Leases | `er`, `status:proposed` | Aligned |
| [#281](https://github.com/justgus/IrisOS/issues/281) | ER-0073 | Proposed | Open | ER-0073 — Comms Transport and Session Objects | `er`, `status:proposed` | Aligned |
| [#282](https://github.com/justgus/IrisOS/issues/282) | ER-0074 | Proposed | Open | ER-0074 — Comms Protocol Objects and Hardware Mapping | `er`, `status:proposed` | Aligned |
| [#283](https://github.com/justgus/IrisOS/issues/283) | ER-0075 | Proposed | Open | ER-0075 — Full Caliper Catalog Expansion | `er`, `status:proposed` | Aligned |
| [#284](https://github.com/justgus/IrisOS/issues/284) | ER-0076 | Proposed | Open | ER-0076 — Runtime Conversion and Compatibility Engine | `er`, `status:proposed` | Aligned |
| [#285](https://github.com/justgus/IrisOS/issues/285) | ER-0077 | Proposed | Open | ER-0077 — Conch Conversion and Inspection Commands | `er`, `status:proposed` | Aligned |
| [#286](https://github.com/justgus/IrisOS/issues/286) | ER-0078 | Verified | Closed | ER-0078 — Refract Constraints and Validation Metadata | `er`, `status:done` | Aligned |
| [#287](https://github.com/justgus/IrisOS/issues/287) | ER-0079 | Verified | Open | ER-0079 — Refract Operation Effects Metadata | `er`, `status:proposed` | Expected `status:done` |
| [#288](https://github.com/justgus/IrisOS/issues/288) | ER-0080 | Verified | Open | ER-0080 — Refract Documentation Objects and Introspection | `er`, `status:proposed` | Expected `status:done` |
| [#290](https://github.com/justgus/IrisOS/issues/290) | AR-0022 | Accepted | Open | AR-0022 — Staged Service Plane Delivery | `ar`, `status:accepted` | Aligned |
| [#291](https://github.com/justgus/IrisOS/issues/291) | AR-0023 | Accepted | Open | AR-0023 — Refract Reflection Profiles | `ar`, `status:accepted` | Aligned |
| [#292](https://github.com/justgus/IrisOS/issues/292) | AR-0024 | Accepted | Open | AR-0024 — Erector Machine And Comms Delivery Tracks | `ar`, `status:accepted` | Aligned |
| [#293](https://github.com/justgus/IrisOS/issues/293) | AR-0025 | Accepted | Open | AR-0025 — Conch And Vizier Interaction Modes | `ar`, `status:accepted` | Aligned |
| [#294](https://github.com/justgus/IrisOS/issues/294) | AR-0026 | Accepted | Open | AR-0026 — Conch Parser Maturity Levels | `ar`, `status:accepted` | Aligned |

## Validation Command

```sh
gh issue list --repo justgus/IrisOS --state all --limit 300 \
  --json number,title,state,labels,url
```

The command was run read-only on 2026-08-17. Results were compared against the inventory in `docs/Airframe-Migration-Audit.md`.

