# Airframe ER to Task Migration

**Date:** 2026-07-07
**Scope:** Local documentation migration
**Epic:** EP-001
**Task:** T-0084

## Boundary

This document maps existing local Engineering Request records to Agile Airframe Task records. It does not move, delete, or rewrite legacy ER files, and it does not query or mutate GitHub.

## Mapping Rules

| Rule | Direction |
| ---- | --------- |
| Source records | Every non-template `docs/ER/ER-*.md` file is treated as one ER source record. |
| Target IDs | Target Task IDs continue from the current Airframe Task index: `T-0089` through `T-0176`. |
| Legacy identity | Each target Task preserves its source ER ID as the legacy identifier. |
| Verified ERs | Existing Verified ERs map to Verified Task records as representation of approved source state. |
| Proposed ERs | Existing Proposed ERs map to Backlog Task records unless the System Engineer assigns them to active Sprint work. |
| GitHub references | Local `GitHub-Issue:` values are preserved as local references only. |
| AR dependencies | Local `AR-Dependencies:` values are preserved as architecture trace links. |

## Summary

| Source Status | Count | Target Handling |
| ------------- | ----- | --------------- |
| Verified | 80 | Verified Task record |
| Proposed | 8 | Backlog Task record |

## Migration Map

| Target Task | Legacy ER | Source Status | Target Status | Local GitHub Ref | AR Dependencies | Title | Source Path |
| ----------- | --------- | ------------- | ------------- | ---------------- | --------------- | ----- | ----------- |
| T-0089 | ER-0001 | Verified | Verified | #82 | - | Service Model and IPC Foundation | docs/ER/ER-0001-Service-Model-and-IPC.md |
| T-0090 | ER-0002 | Verified | Verified | #83 | - | Phase 1 Milestone 1: Refract Schema Registry v0 | docs/ER/ER-0002-Phase1-Schema-Registry.md |
| T-0091 | ER-0003 | Verified | Verified | #84 | - | Phase 1 Milestone 2: Referee Graph Enhancements v0 | docs/ER/ER-0003-Phase1-Referee-Graph-Enhancements.md |
| T-0092 | ER-0004 | Verified | Verified | #85 | - | Phase 1 Integration: Schema Registry + Referee Graph | docs/ER/ER-0004-Phase1-Integration.md |
| T-0093 | ER-0005 | Verified | Verified | #86 | - | Phase 2 Milestone 1: CEO Task Lifecycle v0 | docs/ER/ER-0005-Phase2-Task-Lifecycle.md |
| T-0094 | ER-0006 | Verified | Verified | #87 | - | Phase 2 Milestone 2: Exec Waitables v0 | docs/ER/ER-0006-Phase2-Exec-Waitables.md |
| T-0095 | ER-0007 | Verified | Verified | #88 | - | Phase 2 Integration: CEO + Exec Waitables | docs/ER/ER-0007-Phase2-Integration.md |
| T-0096 | ER-0008 | Verified | Verified | #89 | - | Phase 3 Milestone 1: Conch Shell v0 | docs/ER/ER-0008-Phase3-Conch-Shell.md |
| T-0097 | ER-0009 | Verified | Verified | #90 | - | Phase 3 Milestone 2: Vizier Routing v0 | docs/ER/ER-0009-Phase3-Vizier-Routing.md |
| T-0098 | ER-0010 | Verified | Verified | #91 | - | Phase 3 Milestone 3: Viz Artifacts v0 | docs/ER/ER-0010-Phase3-Viz-Artifacts.md |
| T-0099 | ER-0011 | Verified | Verified | #92 | - | Phase 3 Integration: Conch + Vizier + Viz | docs/ER/ER-0011-Phase3-Integration.md |
| T-0100 | ER-0012 | Verified | Verified | #93 | - | Phase 4 Milestone 1: Demo Object Core | docs/ER/ER-0012-Phase4-Demo-Core.md |
| T-0101 | ER-0013 | Verified | Verified | #94 | - | Phase 4 Milestone 2: Composite Summary Pattern | docs/ER/ER-0013-Phase4-Composite-Summary.md |
| T-0102 | ER-0014 | Verified | Verified | #95 | - | Phase 4 Integration: Demo End-to-End | docs/ER/ER-0014-Phase4-Integration.md |
| T-0103 | ER-0015 | Verified | Verified | #96 | - | Phase 5 Milestone 1: Comms Primitives v0 | docs/ER/ER-0015-Phase5-Comms-Primitives.md |
| T-0104 | ER-0016 | Verified | Verified | #97 | - | Phase 5 Milestone 2: CEO I/O Reactor v0 | docs/ER/ER-0016-Phase5-CEO-IO-Reactor.md |
| T-0105 | ER-0017 | Verified | Verified | #98 | - | Phase 5 Integration: Comms + Reactor | docs/ER/ER-0017-Phase5-Integration.md |
| T-0106 | ER-0018 | Verified | Verified | #99 | - | Phase 6 Milestone 1: Storage Alignment | docs/ER/ER-0018-Phase6-Storage-Alignment.md |
| T-0107 | ER-0019 | Verified | Verified | #100 | - | Phase 6 Milestone 2: Persistence and Migration Tests | docs/ER/ER-0019-Phase6-Persistence-Tests.md |
| T-0108 | ER-0020 | Verified | Verified | #101 | - | Phase 6 Integration: Hardening and Alignment | docs/ER/ER-0020-Phase6-Integration.md |
| T-0109 | ER-0021 | Verified | Verified | #102 | - | Conch Schema and Object Authoring | docs/ER/ER-0021-Conch-Authoring.md |
| T-0110 | ER-0022 | Verified | Verified | #103 | - | Parser Core Framework | docs/ER/ER-0022-Parser-Core-Framework.md |
| T-0111 | ER-0023 | Verified | Verified | #104 | - | Conch Parser Implementation | docs/ER/ER-0023-Conch-Parser-Implementation.md |
| T-0112 | ER-0024 | Verified | Verified | #105 | - | JSON Parser | docs/ER/ER-0024-JSON-Parser.md |
| T-0113 | ER-0025 | Verified | Verified | #106 | - | XML Parser | docs/ER/ER-0025-XML-Parser.md |
| T-0114 | ER-0026 | Verified | Verified | #107 | - | C++ Parser | docs/ER/ER-0026-Cpp-Parser.md |
| T-0115 | ER-0027 | Verified | Verified | #108 | - | Python Parser | docs/ER/ER-0027-Python-Parser.md |
| T-0116 | ER-0028 | Verified | Verified | #109 | - | Conduit Operation Model | docs/ER/ER-0028-Conduit-Operation-Model.md |
| T-0117 | ER-0029 | Verified | Verified | #110 | - | Conduit Dispatch Engine | docs/ER/ER-0029-Conduit-Dispatch-Engine.md |
| T-0118 | ER-0030 | Verified | Verified | #111 | - | Conduit Conch Integration | docs/ER/ER-0030-Conduit-Conch-Integration.md |
| T-0119 | ER-0031 | Verified | Verified | #112 | - | Crate Collections | docs/ER/ER-0031-Crate-Collections.md |
| T-0120 | ER-0032 | Verified | Verified | #113 | - | Astra Math Types | docs/ER/ER-0032-Astra-Math-Types.md |
| T-0121 | ER-0033 | Verified | Verified | #114 | - | Caliper Units and Quantities | docs/ER/ER-0033-Caliper-Units-and-Quantities.md |
| T-0122 | ER-0034 | Verified | Verified | #190 | - | Referee Storage Layout Implementation (v1) | docs/ER/ER-0034-Referee-Storage-Layout-Implementation-v1.md |
| T-0123 | ER-0035 | Verified | Verified | #171 | - | Referee Recovery and Rebuild Workflows | docs/ER/ER-0035-Referee-Recovery-and-Rebuild-Workflows.md |
| T-0124 | ER-0036 | Verified | Verified | #172 | - | Definition Versioning and Migration Hooks | docs/ER/ER-0036-Definition-Versioning-and-Migration-Hooks.md |
| T-0125 | ER-0037 | Verified | Verified | #173 | - | Refract-Native Schema Registry Migration | docs/ER/ER-0037-Refract-Native-Schema-Registry-Migration.md |
| T-0126 | ER-0038 | Verified | Verified | #174 | - | Structured Types: Struct, Packet, Enum | docs/ER/ER-0038-Structured-Types-Struct-Packet-Enum.md |
| T-0127 | ER-0039 | Verified | Verified | #175 | - | Structured Arrays and Collection Integration | docs/ER/ER-0039-Structured-Arrays-and-Collection-Integration.md |
| T-0128 | ER-0040 | Verified | Verified | #176 | - | Generic Type System Core | docs/ER/ER-0040-Generic-Type-System-Core.md |
| T-0129 | ER-0041 | Verified | Verified | #177 | - | Scoped Type Registry | docs/ER/ER-0041-Scoped-Type-Registry.md |
| T-0130 | ER-0042 | Verified | Verified | #178 | - | Core Operations Metadata and Bindings | docs/ER/ER-0042-Core-Operations-Metadata-and-Bindings.md |
| T-0131 | ER-0043 | Verified | Verified | #179 | - | CEO Runtime Hardening | docs/ER/ER-0043-CEO-Runtime-Hardening.md |
| T-0132 | ER-0044 | Verified | Verified | #180 | - | Capability Hooks and Policy Plumbing | docs/ER/ER-0044-Capability-Hooks-and-Policy-Plumbing.md |
| T-0133 | ER-0045 | Verified | Verified | #181 | - | Kernel I/O Primitives Integration | docs/ER/ER-0045-Kernel-IO-Primitives-Integration.md |
| T-0134 | ER-0045.1 | Verified | Verified | #181 | - | Conduit I/O Operations and Schema Definitions | docs/ER/ER-0045.1-Conduit-IO-Operations-and-Schema-Definitions.md |
| T-0135 | ER-0045.2 | Verified | Verified | #181 | - | Conch Commands and Invocation Verification | docs/ER/ER-0045.2-Conch-Commands-and-Invocation-Verification.md |
| T-0136 | ER-0045.3 | Verified | Verified | #181 | - | End-to-End I/O Integration Tests | docs/ER/ER-0045.3-End-to-End-IO-Integration-Tests.md |
| T-0137 | ER-0045.4 | Verified | Verified | #181 | - | Persisted I/O Handles and Conch Aliases | docs/ER/ER-0045.4-Persisted-IO-Handles-and-Conch-Aliases.md |
| T-0138 | ER-0046 | Verified | Verified | #182 | - | v1 Kernel Demo Integration and Roadmap | docs/ER/ER-0046-v1-Kernel-Demo-Integration-and-Roadmap.md |
| T-0139 | ER-0047 | Verified | Verified | #183 | - | Userland Core Utilities Suite | docs/ER/ER-0047-Userland-Core-Utilities-Suite.md |
| T-0140 | ER-0047.1 | Verified | Verified | #182 | - | Conch Session Operation Model | docs/ER/ER-0047.1-Conch-Session-Operation-Model.md |
| T-0141 | ER-0047.2 | Verified | Verified | #182 | - | Core Command Migration | docs/ER/ER-0047.2-Core-Command-Migration.md |
| T-0142 | ER-0047.3 | Verified | Verified | #182 | - | Utility Command Migration | docs/ER/ER-0047.3-Utility-Command-Migration.md |
| T-0143 | ER-0047.4 | Verified | Verified | #182 | - | Command Alias Catalog | docs/ER/ER-0047.4-Command-Alias-Catalog.md |
| T-0144 | ER-0048 | Verified | Verified | #184 | - | Referee Import/Export Tools | docs/ER/ER-0048-Referee-Import-Export-Tools.md |
| T-0145 | ER-0049 | Verified | Verified | #185 | - | Object Graph Bundles and Packages | docs/ER/ER-0049-Object-Graph-Bundles-and-Packages.md |
| T-0146 | ER-0050 | Verified | Verified | #186 | - | Schema Migration Tools | docs/ER/ER-0050-Schema-Migration-Tools.md |
| T-0147 | ER-0051 | Verified | Verified | #187 | - | CEO Profiling and Trace Utilities | docs/ER/ER-0051-CEO-Profiling-and-Trace-Utilities.md |
| T-0148 | ER-0052 | Verified | Verified | #188 | - | Conch Debug and Inspection Tools | docs/ER/ER-0052-Conch-Debug-and-Inspection-Tools.md |
| T-0149 | ER-0053 | Verified | Verified | #189 | - | v2 Integration and Usability Demo | docs/ER/ER-0053-v2-Integration-and-Usability-Demo.md |
| T-0150 | ER-0054 | Verified | Verified | #184 | - | Conch Namespace Navigation | docs/ER/ER-0054-Conch-Namespace-Navigation.md |
| T-0151 | ER-0055 | Verified | Verified | #262 | - | Refract Inheritance and Interface Metadata | docs/ER/ER-0055-Refract-Inheritance-and-Interface-Metadata.md |
| T-0152 | ER-0056 | Verified | Verified | #263 | - | Conch Typed AST and Shell Decomposition | docs/ER/ER-0056-Conch-Typed-AST-and-Shell-Decomposition.md |
| T-0153 | ER-0057 | Verified | Verified | #264 | - | Service Host Lifecycle and Persistent Registry | docs/ER/ER-0057-Service-Host-Lifecycle-and-Persistent-Registry.md |
| T-0154 | ER-0058 | Verified | Verified | #265 | - | Capability Context Objects and Persistence | docs/ER/ER-0058-Capability-Context-Objects-and-Persistence.md |
| T-0155 | ER-0059 | Verified | Verified | #266 | - | CEO Task Capability Attachment | docs/ER/ER-0059-CEO-Task-Capability-Attachment.md |
| T-0156 | ER-0060 | Verified | Verified | #267 | - | Service Boundary Capability Enforcement | docs/ER/ER-0060-Service-Boundary-Capability-Enforcement.md |
| T-0157 | ER-0061 | Verified | Verified | #268 | - | Sandbox Identity and Service Isolation Hooks | docs/ER/ER-0061-Sandbox-Identity-and-Service-Isolation-Hooks.md |
| T-0158 | ER-0062 | Verified | Verified | #269 | - | Memory Service Baseline | docs/ER/ER-0062-Memory-Service-Baseline.md |
| T-0159 | ER-0063 | Verified | Verified | #270 | AR-0009, AR-0011, AR-0012, AR-0025 | Referee Graph Watch and Change-Feed API | docs/ER/ER-0063-Referee-Graph-Watch-and-Change-Feed-API.md |
| T-0160 | ER-0064 | Verified | Verified | #271 | AR-0011, AR-0012, AR-0013, AR-0025 | Vizier Relationship-Pattern Routing | docs/ER/ER-0064-Vizier-Relationship-Pattern-Routing.md |
| T-0161 | ER-0065 | Verified | Verified | #272 | AR-0011, AR-0012, AR-0013, AR-0025 | Task Visualization Objects and Task Conchos | docs/ER/ER-0065-Task-Visualization-Objects-and-Task-Conchos.md |
| T-0162 | ER-0066 | Verified | Verified | #273 | AR-0011, AR-0012, AR-0013, AR-0025 | Observer-Driven Conch Session Growth | docs/ER/ER-0066-Observer-Driven-Conch-Session-Growth.md |
| T-0163 | ER-0067 | Verified | Verified | #274 | AR-0015, AR-0026 | Reusable Conch Grammar API | docs/ER/ER-0067-Reusable-Conch-Grammar-API.md |
| T-0164 | ER-0068 | Verified | Verified | #275 | AR-0015, AR-0026 | Batch Execution and Non-Interactive Parser Integration | docs/ER/ER-0068-Batch-Execution-and-Non-Interactive-Parser-Integration.md |
| T-0165 | ER-0069 | Verified | Verified | #276 | AR-0015, AR-0026 | Shared Parser Regression Harness | docs/ER/ER-0069-Shared-Parser-Regression-Harness.md |
| T-0166 | ER-0070 | Proposed | Backlog | #278 | AR-0008, AR-0010, AR-0024 | Machine Representation Primitives | docs/ER/ER-0070-Machine-Representation-Primitives.md |
| T-0167 | ER-0071 | Proposed | Backlog | #279 | AR-0008, AR-0010, AR-0024 | Machine Descriptors and Resource Facts | docs/ER/ER-0071-Machine-Descriptors-and-Resource-Facts.md |
| T-0168 | ER-0072 | Proposed | Backlog | #280 | AR-0008, AR-0010, AR-0024 | Machine Handles and Leases | docs/ER/ER-0072-Machine-Handles-and-Leases.md |
| T-0169 | ER-0073 | Proposed | Backlog | #281 | AR-0010, AR-0024 | Comms Transport and Session Objects | docs/ER/ER-0073-Comms-Transport-and-Session-Objects.md |
| T-0170 | ER-0074 | Proposed | Backlog | #282 | AR-0010, AR-0024 | Comms Protocol Objects and Hardware Mapping | docs/ER/ER-0074-Comms-Protocol-Objects-and-Hardware-Mapping.md |
| T-0171 | ER-0075 | Proposed | Backlog | #283 | AR-0019 | Full Caliper Catalog Expansion | docs/ER/ER-0075-Full-Caliper-Catalog-Expansion.md |
| T-0172 | ER-0076 | Proposed | Backlog | #284 | AR-0019 | Runtime Conversion and Compatibility Engine | docs/ER/ER-0076-Runtime-Conversion-and-Compatibility-Engine.md |
| T-0173 | ER-0077 | Proposed | Backlog | #285 | AR-0019 | Conch Conversion and Inspection Commands | docs/ER/ER-0077-Conch-Conversion-and-Inspection-Commands.md |
| T-0174 | ER-0078 | Verified | Verified | #286 | - | Refract Constraints and Validation Metadata | docs/ER/ER-0078-Refract-Constraints-and-Validation-Metadata.md |
| T-0175 | ER-0079 | Verified | Verified | #287 | - | Refract Operation Effects Metadata | docs/ER/ER-0079-Refract-Operation-Effects-Metadata.md |
| T-0176 | ER-0080 | Verified | Verified | #288 | - | Refract Documentation Objects and Introspection | docs/ER/ER-0080-Refract-Documentation-Objects-and-Introspection.md |

## Validation Commands

The migration map used these local read-only commands:

```sh
rg -n "^# ER-|^- Status:|^GitHub-Issue:|^AR-Dependencies:" docs/ER
ruby -e '...local ER to Task mapping parser...'
```

## Generated Canonical Records

Canonical Task records were generated from this map:

- Verified Task records: `docs/Tasks/Verified/`
- Backlog Task records: `docs/Tasks/Backlog/`

The generated Task records preserve the complete legacy ER body in a `Preserved Legacy ER Content` section.

## Caveats

- The Verified target status represents existing System Engineer-approved ER source state; it is not a new verification action.
- Shared local GitHub issue references are preserved as-is for later mapping review.
