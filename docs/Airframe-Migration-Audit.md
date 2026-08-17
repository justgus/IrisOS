# Airframe Migration Local Audit

**Date:** 2026-07-07
**Scope:** Local documentation only
**Epic:** EP-001
**Task:** T-0081

## Boundary

This audit inventories the current local AR, ER, and DR documentation for the Agile Airframe migration. It does not query GitHub and does not mutate local source code, build files, generated files, or GitHub issue state.

GitHub fields in this document are local references copied from document front matter only. Live GitHub issue title, state, and labels are intentionally not captured in this local-only pass.

## Summary

| Record Type | Count | Status Breakdown | Local GitHub Gaps |
| ----------- | ----- | ---------------- | ----------------- |
| AR | 26 | Accepted: 11; Implemented: 15 | 4 local `#TBD` refs |
| ER | 88 | Proposed: 8; Verified: 80 | 0 |
| DR | 1 | Verified: 1 | 0 |

## Local Consistency Checks

| Check | Result |
| ----- | ------ |
| ER documents represented in `docs/ER/ER-Status.md` | Pass |
| ER status ledger entries with matching document files | Pass |
| ER document statuses matching `docs/ER/ER-Status.md` | Pass |
| DR documents represented in `docs/DR/DR-Status.md` | Pass |
| DR status ledger entries with matching document files | Pass |
| DR document statuses matching `docs/DR/DR-Status.md` | Pass |

## Local Migration Diagnostics

| Diagnostic | Records |
| ---------- | ------- |
| AR records with local `GitHub-Issue: #TBD` | AR-0018, AR-0019, AR-0020, AR-0021 |
| Multiple ER records sharing one local GitHub issue reference | `#181`: ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4 |
| Multiple ER records sharing one local GitHub issue reference | `#182`: ER-0046, ER-0047.1, ER-0047.2, ER-0047.3, ER-0047.4 |
| Multiple ER records sharing one local GitHub issue reference | `#184`: ER-0048, ER-0054 |

Shared local GitHub issue references are recorded as diagnostics, not errors. They may represent parent or tracking issues and require System Engineer review before any GitHub mapping is normalized.

## Inventory

| Source ID | Local Status | Local GitHub Ref | AR Dependencies | Title | Target Airframe Handling | Source Path |
| --------- | ------------ | ---------------- | --------------- | ----- | ------------------------ | ----------- |
| AR-0001 | Accepted | #65 | - | Expected-Style Error Model | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0001-Error-Model.md |
| AR-0002 | Implemented | #66 | - | Object Identity and Versioning Semantics | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0002-Object-Identity.md |
| AR-0003 | Implemented | #67 | - | Storage Layout Strategy | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0003-Storage-Layout-Strategy.md |
| AR-0004 | Implemented | #68 | - | Index and Graph Storage Strategy (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0004-Index-and-Graph-Storage.md |
| AR-0005 | Implemented | #69 | - | Service Plane Model (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0005-Service-Plane-Model.md |
| AR-0006 | Implemented | #70 | - | CEO/Exec Runtime Model (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0006-CEO-Exec-Runtime-Model.md |
| AR-0007 | Implemented | #71 | - | Refract Reflection Graph (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0007-Refract-Reflection-Graph.md |
| AR-0008 | Implemented | #72 | - | Erector Subsystems (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0008-Erector-Subsystems.md |
| AR-0009 | Implemented | #73 | - | Referee Object Store (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0009-Referee-Object-Store.md |
| AR-0010 | Implemented | #74 | - | Comms Subsystem (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0010-Comms-Subsystem.md |
| AR-0011 | Accepted | #75 | - | Vizier Interpretation Layer (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0011-Vizier-Interpretation-Layer.md |
| AR-0012 | Accepted | #76 | - | Conch Shell and Concho Views (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0012-Conch-Shell-and-Conchos.md |
| AR-0013 | Implemented | #77 | - | Viz Display Subsystem (Recommendation) | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0013-Viz-Display-Subsystem.md |
| AR-0014 | Implemented | #78 | - | Conch Schema and Object Authoring | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0014-Conch-Authoring.md |
| AR-0015 | Accepted | #79 | - | Conch Parser and Syntax | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0015-Conch-Parser-and-Syntax.md |
| AR-0016 | Implemented | #80 | - | Operations and Dispatch Model | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0016-Operations-Dispatch.md |
| AR-0017 | Accepted | #81 | - | Collections, Math Types, and Units | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0017-Collections-Math-Units.md |
| AR-0018 | Implemented | #TBD | - | Generic Type System | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0018-Generic-Type-System.md |
| AR-0019 | Accepted | #TBD | - | Caliper Unit Catalog | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0019-Caliper-Unit-Catalog.md |
| AR-0020 | Implemented | #TBD | - | Core Type Operations and Rendering | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0020-Core-Type-Ops-and-Rendering.md |
| AR-0021 | Implemented | #TBD | - | Structured Types, Enums, and Packets | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0021-Structured-Types-Enums-Packets.md |
| AR-0022 | Accepted | #290 | - | Staged Service Plane Delivery | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0022-Staged-Service-Plane-Delivery.md |
| AR-0023 | Accepted | #291 | - | Refract Reflection Profiles | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0023-Refract-Reflection-Profiles.md |
| AR-0024 | Accepted | #292 | - | Erector Machine And Comms Delivery Tracks | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0024-Erector-Machine-and-Comms-Delivery-Tracks.md |
| AR-0025 | Accepted | #293 | - | Conch And Vizier Interaction Modes | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0025-Conch-and-Vizier-Interaction-Modes.md |
| AR-0026 | Accepted | #294 | - | Conch Parser Maturity Levels | Architecture record; preserve AR namespace | docs/AR/accepted/AR-0026-Conch-Parser-Maturity-Levels.md |
| ER-0001 | Verified | #82 | - | Service Model and IPC Foundation | Task; preserve legacy ER ID | docs/ER/ER-0001-Service-Model-and-IPC.md |
| ER-0002 | Verified | #83 | - | Phase 1 Milestone 1: Refract Schema Registry v0 | Task; preserve legacy ER ID | docs/ER/ER-0002-Phase1-Schema-Registry.md |
| ER-0003 | Verified | #84 | - | Phase 1 Milestone 2: Referee Graph Enhancements v0 | Task; preserve legacy ER ID | docs/ER/ER-0003-Phase1-Referee-Graph-Enhancements.md |
| ER-0004 | Verified | #85 | - | Phase 1 Integration: Schema Registry + Referee Graph | Task; preserve legacy ER ID | docs/ER/ER-0004-Phase1-Integration.md |
| ER-0005 | Verified | #86 | - | Phase 2 Milestone 1: CEO Task Lifecycle v0 | Task; preserve legacy ER ID | docs/ER/ER-0005-Phase2-Task-Lifecycle.md |
| ER-0006 | Verified | #87 | - | Phase 2 Milestone 2: Exec Waitables v0 | Task; preserve legacy ER ID | docs/ER/ER-0006-Phase2-Exec-Waitables.md |
| ER-0007 | Verified | #88 | - | Phase 2 Integration: CEO + Exec Waitables | Task; preserve legacy ER ID | docs/ER/ER-0007-Phase2-Integration.md |
| ER-0008 | Verified | #89 | - | Phase 3 Milestone 1: Conch Shell v0 | Task; preserve legacy ER ID | docs/ER/ER-0008-Phase3-Conch-Shell.md |
| ER-0009 | Verified | #90 | - | Phase 3 Milestone 2: Vizier Routing v0 | Task; preserve legacy ER ID | docs/ER/ER-0009-Phase3-Vizier-Routing.md |
| ER-0010 | Verified | #91 | - | Phase 3 Milestone 3: Viz Artifacts v0 | Task; preserve legacy ER ID | docs/ER/ER-0010-Phase3-Viz-Artifacts.md |
| ER-0011 | Verified | #92 | - | Phase 3 Integration: Conch + Vizier + Viz | Task; preserve legacy ER ID | docs/ER/ER-0011-Phase3-Integration.md |
| ER-0012 | Verified | #93 | - | Phase 4 Milestone 1: Demo Object Core | Task; preserve legacy ER ID | docs/ER/ER-0012-Phase4-Demo-Core.md |
| ER-0013 | Verified | #94 | - | Phase 4 Milestone 2: Composite Summary Pattern | Task; preserve legacy ER ID | docs/ER/ER-0013-Phase4-Composite-Summary.md |
| ER-0014 | Verified | #95 | - | Phase 4 Integration: Demo End-to-End | Task; preserve legacy ER ID | docs/ER/ER-0014-Phase4-Integration.md |
| ER-0015 | Verified | #96 | - | Phase 5 Milestone 1: Comms Primitives v0 | Task; preserve legacy ER ID | docs/ER/ER-0015-Phase5-Comms-Primitives.md |
| ER-0016 | Verified | #97 | - | Phase 5 Milestone 2: CEO I/O Reactor v0 | Task; preserve legacy ER ID | docs/ER/ER-0016-Phase5-CEO-IO-Reactor.md |
| ER-0017 | Verified | #98 | - | Phase 5 Integration: Comms + Reactor | Task; preserve legacy ER ID | docs/ER/ER-0017-Phase5-Integration.md |
| ER-0018 | Verified | #99 | - | Phase 6 Milestone 1: Storage Alignment | Task; preserve legacy ER ID | docs/ER/ER-0018-Phase6-Storage-Alignment.md |
| ER-0019 | Verified | #100 | - | Phase 6 Milestone 2: Persistence and Migration Tests | Task; preserve legacy ER ID | docs/ER/ER-0019-Phase6-Persistence-Tests.md |
| ER-0020 | Verified | #101 | - | Phase 6 Integration: Hardening and Alignment | Task; preserve legacy ER ID | docs/ER/ER-0020-Phase6-Integration.md |
| ER-0021 | Verified | #102 | - | Conch Schema and Object Authoring | Task; preserve legacy ER ID | docs/ER/ER-0021-Conch-Authoring.md |
| ER-0022 | Verified | #103 | - | Parser Core Framework | Task; preserve legacy ER ID | docs/ER/ER-0022-Parser-Core-Framework.md |
| ER-0023 | Verified | #104 | - | Conch Parser Implementation | Task; preserve legacy ER ID | docs/ER/ER-0023-Conch-Parser-Implementation.md |
| ER-0024 | Verified | #105 | - | JSON Parser | Task; preserve legacy ER ID | docs/ER/ER-0024-JSON-Parser.md |
| ER-0025 | Verified | #106 | - | XML Parser | Task; preserve legacy ER ID | docs/ER/ER-0025-XML-Parser.md |
| ER-0026 | Verified | #107 | - | C++ Parser | Task; preserve legacy ER ID | docs/ER/ER-0026-Cpp-Parser.md |
| ER-0027 | Verified | #108 | - | Python Parser | Task; preserve legacy ER ID | docs/ER/ER-0027-Python-Parser.md |
| ER-0028 | Verified | #109 | - | Conduit Operation Model | Task; preserve legacy ER ID | docs/ER/ER-0028-Conduit-Operation-Model.md |
| ER-0029 | Verified | #110 | - | Conduit Dispatch Engine | Task; preserve legacy ER ID | docs/ER/ER-0029-Conduit-Dispatch-Engine.md |
| ER-0030 | Verified | #111 | - | Conduit Conch Integration | Task; preserve legacy ER ID | docs/ER/ER-0030-Conduit-Conch-Integration.md |
| ER-0031 | Verified | #112 | - | Crate Collections | Task; preserve legacy ER ID | docs/ER/ER-0031-Crate-Collections.md |
| ER-0032 | Verified | #113 | - | Astra Math Types | Task; preserve legacy ER ID | docs/ER/ER-0032-Astra-Math-Types.md |
| ER-0033 | Verified | #114 | - | Caliper Units and Quantities | Task; preserve legacy ER ID | docs/ER/ER-0033-Caliper-Units-and-Quantities.md |
| ER-0034 | Verified | #190 | - | Referee Storage Layout Implementation (v1) | Task; preserve legacy ER ID | docs/ER/ER-0034-Referee-Storage-Layout-Implementation-v1.md |
| ER-0035 | Verified | #171 | - | Referee Recovery and Rebuild Workflows | Task; preserve legacy ER ID | docs/ER/ER-0035-Referee-Recovery-and-Rebuild-Workflows.md |
| ER-0036 | Verified | #172 | - | Definition Versioning and Migration Hooks | Task; preserve legacy ER ID | docs/ER/ER-0036-Definition-Versioning-and-Migration-Hooks.md |
| ER-0037 | Verified | #173 | - | Refract-Native Schema Registry Migration | Task; preserve legacy ER ID | docs/ER/ER-0037-Refract-Native-Schema-Registry-Migration.md |
| ER-0038 | Verified | #174 | - | Structured Types: Struct, Packet, Enum | Task; preserve legacy ER ID | docs/ER/ER-0038-Structured-Types-Struct-Packet-Enum.md |
| ER-0039 | Verified | #175 | - | Structured Arrays and Collection Integration | Task; preserve legacy ER ID | docs/ER/ER-0039-Structured-Arrays-and-Collection-Integration.md |
| ER-0040 | Verified | #176 | - | Generic Type System Core | Task; preserve legacy ER ID | docs/ER/ER-0040-Generic-Type-System-Core.md |
| ER-0041 | Verified | #177 | - | Scoped Type Registry | Task; preserve legacy ER ID | docs/ER/ER-0041-Scoped-Type-Registry.md |
| ER-0042 | Verified | #178 | - | Core Operations Metadata and Bindings | Task; preserve legacy ER ID | docs/ER/ER-0042-Core-Operations-Metadata-and-Bindings.md |
| ER-0043 | Verified | #179 | - | CEO Runtime Hardening | Task; preserve legacy ER ID | docs/ER/ER-0043-CEO-Runtime-Hardening.md |
| ER-0044 | Verified | #180 | - | Capability Hooks and Policy Plumbing | Task; preserve legacy ER ID | docs/ER/ER-0044-Capability-Hooks-and-Policy-Plumbing.md |
| ER-0045 | Verified | #181 | - | Kernel I/O Primitives Integration | Task; preserve legacy ER ID | docs/ER/ER-0045-Kernel-IO-Primitives-Integration.md |
| ER-0045.1 | Verified | #181 | - | Conduit I/O Operations and Schema Definitions | Task; preserve legacy ER ID | docs/ER/ER-0045.1-Conduit-IO-Operations-and-Schema-Definitions.md |
| ER-0045.2 | Verified | #181 | - | Conch Commands and Invocation Verification | Task; preserve legacy ER ID | docs/ER/ER-0045.2-Conch-Commands-and-Invocation-Verification.md |
| ER-0045.3 | Verified | #181 | - | End-to-End I/O Integration Tests | Task; preserve legacy ER ID | docs/ER/ER-0045.3-End-to-End-IO-Integration-Tests.md |
| ER-0045.4 | Verified | #181 | - | Persisted I/O Handles and Conch Aliases | Task; preserve legacy ER ID | docs/ER/ER-0045.4-Persisted-IO-Handles-and-Conch-Aliases.md |
| ER-0046 | Verified | #182 | - | v1 Kernel Demo Integration and Roadmap | Task; preserve legacy ER ID | docs/ER/ER-0046-v1-Kernel-Demo-Integration-and-Roadmap.md |
| ER-0047 | Verified | #183 | - | Userland Core Utilities Suite | Task; preserve legacy ER ID | docs/ER/ER-0047-Userland-Core-Utilities-Suite.md |
| ER-0047.1 | Verified | #182 | - | Conch Session Operation Model | Task; preserve legacy ER ID | docs/ER/ER-0047.1-Conch-Session-Operation-Model.md |
| ER-0047.2 | Verified | #182 | - | Core Command Migration | Task; preserve legacy ER ID | docs/ER/ER-0047.2-Core-Command-Migration.md |
| ER-0047.3 | Verified | #182 | - | Utility Command Migration | Task; preserve legacy ER ID | docs/ER/ER-0047.3-Utility-Command-Migration.md |
| ER-0047.4 | Verified | #182 | - | Command Alias Catalog | Task; preserve legacy ER ID | docs/ER/ER-0047.4-Command-Alias-Catalog.md |
| ER-0048 | Verified | #184 | - | Referee Import/Export Tools | Task; preserve legacy ER ID | docs/ER/ER-0048-Referee-Import-Export-Tools.md |
| ER-0049 | Verified | #185 | - | Object Graph Bundles and Packages | Task; preserve legacy ER ID | docs/ER/ER-0049-Object-Graph-Bundles-and-Packages.md |
| ER-0050 | Verified | #186 | - | Schema Migration Tools | Task; preserve legacy ER ID | docs/ER/ER-0050-Schema-Migration-Tools.md |
| ER-0051 | Verified | #187 | - | CEO Profiling and Trace Utilities | Task; preserve legacy ER ID | docs/ER/ER-0051-CEO-Profiling-and-Trace-Utilities.md |
| ER-0052 | Verified | #188 | - | Conch Debug and Inspection Tools | Task; preserve legacy ER ID | docs/ER/ER-0052-Conch-Debug-and-Inspection-Tools.md |
| ER-0053 | Verified | #189 | - | v2 Integration and Usability Demo | Task; preserve legacy ER ID | docs/ER/ER-0053-v2-Integration-and-Usability-Demo.md |
| ER-0054 | Verified | #184 | - | Conch Namespace Navigation | Task; preserve legacy ER ID | docs/ER/ER-0054-Conch-Namespace-Navigation.md |
| ER-0055 | Verified | #262 | - | Refract Inheritance and Interface Metadata | Task; preserve legacy ER ID | docs/ER/ER-0055-Refract-Inheritance-and-Interface-Metadata.md |
| ER-0056 | Verified | #263 | - | Conch Typed AST and Shell Decomposition | Task; preserve legacy ER ID | docs/ER/ER-0056-Conch-Typed-AST-and-Shell-Decomposition.md |
| ER-0057 | Verified | #264 | - | Service Host Lifecycle and Persistent Registry | Task; preserve legacy ER ID | docs/ER/ER-0057-Service-Host-Lifecycle-and-Persistent-Registry.md |
| ER-0058 | Verified | #265 | - | Capability Context Objects and Persistence | Task; preserve legacy ER ID | docs/ER/ER-0058-Capability-Context-Objects-and-Persistence.md |
| ER-0059 | Verified | #266 | - | CEO Task Capability Attachment | Task; preserve legacy ER ID | docs/ER/ER-0059-CEO-Task-Capability-Attachment.md |
| ER-0060 | Verified | #267 | - | Service Boundary Capability Enforcement | Task; preserve legacy ER ID | docs/ER/ER-0060-Service-Boundary-Capability-Enforcement.md |
| ER-0061 | Verified | #268 | - | Sandbox Identity and Service Isolation Hooks | Task; preserve legacy ER ID | docs/ER/ER-0061-Sandbox-Identity-and-Service-Isolation-Hooks.md |
| ER-0062 | Verified | #269 | - | Memory Service Baseline | Task; preserve legacy ER ID | docs/ER/ER-0062-Memory-Service-Baseline.md |
| ER-0063 | Verified | #270 | AR-0009, AR-0011, AR-0012, AR-0025 | Referee Graph Watch and Change-Feed API | Task; preserve legacy ER ID | docs/ER/ER-0063-Referee-Graph-Watch-and-Change-Feed-API.md |
| ER-0064 | Verified | #271 | AR-0011, AR-0012, AR-0013, AR-0025 | Vizier Relationship-Pattern Routing | Task; preserve legacy ER ID | docs/ER/ER-0064-Vizier-Relationship-Pattern-Routing.md |
| ER-0065 | Verified | #272 | AR-0011, AR-0012, AR-0013, AR-0025 | Task Visualization Objects and Task Conchos | Task; preserve legacy ER ID | docs/ER/ER-0065-Task-Visualization-Objects-and-Task-Conchos.md |
| ER-0066 | Verified | #273 | AR-0011, AR-0012, AR-0013, AR-0025 | Observer-Driven Conch Session Growth | Task; preserve legacy ER ID | docs/ER/ER-0066-Observer-Driven-Conch-Session-Growth.md |
| ER-0067 | Verified | #274 | AR-0015, AR-0026 | Reusable Conch Grammar API | Task; preserve legacy ER ID | docs/ER/ER-0067-Reusable-Conch-Grammar-API.md |
| ER-0068 | Verified | #275 | AR-0015, AR-0026 | Batch Execution and Non-Interactive Parser Integration | Task; preserve legacy ER ID | docs/ER/ER-0068-Batch-Execution-and-Non-Interactive-Parser-Integration.md |
| ER-0069 | Verified | #276 | AR-0015, AR-0026 | Shared Parser Regression Harness | Task; preserve legacy ER ID | docs/ER/ER-0069-Shared-Parser-Regression-Harness.md |
| ER-0070 | Proposed | #278 | AR-0008, AR-0010, AR-0024 | Machine Representation Primitives | Task; preserve legacy ER ID | docs/ER/ER-0070-Machine-Representation-Primitives.md |
| ER-0071 | Proposed | #279 | AR-0008, AR-0010, AR-0024 | Machine Descriptors and Resource Facts | Task; preserve legacy ER ID | docs/ER/ER-0071-Machine-Descriptors-and-Resource-Facts.md |
| ER-0072 | Proposed | #280 | AR-0008, AR-0010, AR-0024 | Machine Handles and Leases | Task; preserve legacy ER ID | docs/ER/ER-0072-Machine-Handles-and-Leases.md |
| ER-0073 | Proposed | #281 | AR-0010, AR-0024 | Comms Transport and Session Objects | Task; preserve legacy ER ID | docs/ER/ER-0073-Comms-Transport-and-Session-Objects.md |
| ER-0074 | Proposed | #282 | AR-0010, AR-0024 | Comms Protocol Objects and Hardware Mapping | Task; preserve legacy ER ID | docs/ER/ER-0074-Comms-Protocol-Objects-and-Hardware-Mapping.md |
| ER-0075 | Proposed | #283 | AR-0019 | Full Caliper Catalog Expansion | Task; preserve legacy ER ID | docs/ER/ER-0075-Full-Caliper-Catalog-Expansion.md |
| ER-0076 | Proposed | #284 | AR-0019 | Runtime Conversion and Compatibility Engine | Task; preserve legacy ER ID | docs/ER/ER-0076-Runtime-Conversion-and-Compatibility-Engine.md |
| ER-0077 | Proposed | #285 | AR-0019 | Conch Conversion and Inspection Commands | Task; preserve legacy ER ID | docs/ER/ER-0077-Conch-Conversion-and-Inspection-Commands.md |
| ER-0078 | Verified | #286 | - | Refract Constraints and Validation Metadata | Task; preserve legacy ER ID | docs/ER/ER-0078-Refract-Constraints-and-Validation-Metadata.md |
| ER-0079 | Verified | #287 | - | Refract Operation Effects Metadata | Task; preserve legacy ER ID | docs/ER/ER-0079-Refract-Operation-Effects-Metadata.md |
| ER-0080 | Verified | #288 | - | Refract Documentation Objects and Introspection | Task; preserve legacy ER ID | docs/ER/ER-0080-Refract-Documentation-Objects-and-Introspection.md |
| DR-0001 | Verified | #116 | AR-0012, AR-0016 | Conch Compare Alias Resolution | Issue; preserve legacy DR ID | docs/DR/DR-0001-Conch-Compare-Alias-Resolution.md |

## Validation Commands

The local audit used these read-only commands:

```sh
find docs/AR docs/ER docs/DR -type f -name 'AR-*.md' -o -name 'ER-*.md' -o -name 'DR-*.md'
rg -n "^# (AR|ER|DR)-|^- Status:|^GitHub-Issue:|^AR-Dependencies:|^- (ER|DR)-[0-9]" docs/AR docs/ER docs/DR
ruby -e '...local AR/ER/DR inventory parser...'
```

## Caveats

- The original 2026-07-07 pass was local-only. The follow-up live, read-only comparison is recorded in `docs/Airframe-GitHub-Live-Audit.md`.
- The AR inventory has no AR status ledger equivalent; statuses were read from each AR document.
- ER and DR ledger checks validate local status consistency only.
- Shared local GitHub issue references require System Engineer review before any mapping change.
