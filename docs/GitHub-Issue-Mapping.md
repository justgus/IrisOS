# GitHub Issue Mapping

**Date:** 2026-07-07
**Scope:** Local documentation migration
**Epic:** EP-001
**Task:** T-0086

## Boundary

This mapping preserves local `GitHub-Issue:` references from AR, ER, and DR documents during the Agile Airframe migration. It does not query GitHub and does not mutate issue titles, labels, bodies, or state.

## Summary

| Mapping Area | Count | Notes |
| ------------ | ----- | ----- |
| ER-derived Task mappings | 88 | Local refs preserved from ER front matter. |
| DR-derived Issue mappings | 1 | Local ref preserved from DR front matter. |
| AR records with `#TBD` local refs | 0 | ARs remain architecture records and are not Task/Issue mappings. |

## Local Mapping Diagnostics

| Diagnostic | Records |
| ---------- | ------- |
| AR records with local `GitHub-Issue: #TBD` | None; AR-0018 through AR-0021 map to #301 through #304. |
| Multiple ER records sharing one local GitHub issue reference | `#181`: ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4 |
| Multiple ER records sharing one local GitHub issue reference | `#182`: ER-0046, ER-0047.1, ER-0047.2, ER-0047.3, ER-0047.4 |
| Multiple ER records sharing one local GitHub issue reference | `#184`: ER-0048, ER-0054 |

Shared local GitHub issue references are preserved as-is until the System Engineer approves a GitHub synchronization plan.

## Airframe Task and Issue Mapping

| Airframe ID | Legacy ID | Local GitHub Ref | Title |
| ----------- | --------- | ---------------- | ----- |
| T-0089 | ER-0001 | #82 | Service Model and IPC Foundation |
| T-0090 | ER-0002 | #83 | Phase 1 Milestone 1: Refract Schema Registry v0 |
| T-0091 | ER-0003 | #84 | Phase 1 Milestone 2: Referee Graph Enhancements v0 |
| T-0092 | ER-0004 | #85 | Phase 1 Integration: Schema Registry + Referee Graph |
| T-0093 | ER-0005 | #86 | Phase 2 Milestone 1: CEO Task Lifecycle v0 |
| T-0094 | ER-0006 | #87 | Phase 2 Milestone 2: Exec Waitables v0 |
| T-0095 | ER-0007 | #88 | Phase 2 Integration: CEO + Exec Waitables |
| T-0096 | ER-0008 | #89 | Phase 3 Milestone 1: Conch Shell v0 |
| T-0097 | ER-0009 | #90 | Phase 3 Milestone 2: Vizier Routing v0 |
| T-0098 | ER-0010 | #91 | Phase 3 Milestone 3: Viz Artifacts v0 |
| T-0099 | ER-0011 | #92 | Phase 3 Integration: Conch + Vizier + Viz |
| T-0100 | ER-0012 | #93 | Phase 4 Milestone 1: Demo Object Core |
| T-0101 | ER-0013 | #94 | Phase 4 Milestone 2: Composite Summary Pattern |
| T-0102 | ER-0014 | #95 | Phase 4 Integration: Demo End-to-End |
| T-0103 | ER-0015 | #96 | Phase 5 Milestone 1: Comms Primitives v0 |
| T-0104 | ER-0016 | #97 | Phase 5 Milestone 2: CEO I/O Reactor v0 |
| T-0105 | ER-0017 | #98 | Phase 5 Integration: Comms + Reactor |
| T-0106 | ER-0018 | #99 | Phase 6 Milestone 1: Storage Alignment |
| T-0107 | ER-0019 | #100 | Phase 6 Milestone 2: Persistence and Migration Tests |
| T-0108 | ER-0020 | #101 | Phase 6 Integration: Hardening and Alignment |
| T-0109 | ER-0021 | #102 | Conch Schema and Object Authoring |
| T-0110 | ER-0022 | #103 | Parser Core Framework |
| T-0111 | ER-0023 | #104 | Conch Parser Implementation |
| T-0112 | ER-0024 | #105 | JSON Parser |
| T-0113 | ER-0025 | #106 | XML Parser |
| T-0114 | ER-0026 | #107 | C++ Parser |
| T-0115 | ER-0027 | #108 | Python Parser |
| T-0116 | ER-0028 | #109 | Conduit Operation Model |
| T-0117 | ER-0029 | #110 | Conduit Dispatch Engine |
| T-0118 | ER-0030 | #111 | Conduit Conch Integration |
| T-0119 | ER-0031 | #112 | Crate Collections |
| T-0120 | ER-0032 | #113 | Astra Math Types |
| T-0121 | ER-0033 | #114 | Caliper Units and Quantities |
| T-0122 | ER-0034 | #190 | Referee Storage Layout Implementation (v1) |
| T-0123 | ER-0035 | #171 | Referee Recovery and Rebuild Workflows |
| T-0124 | ER-0036 | #172 | Definition Versioning and Migration Hooks |
| T-0125 | ER-0037 | #173 | Refract-Native Schema Registry Migration |
| T-0126 | ER-0038 | #174 | Structured Types: Struct, Packet, Enum |
| T-0127 | ER-0039 | #175 | Structured Arrays and Collection Integration |
| T-0128 | ER-0040 | #176 | Generic Type System Core |
| T-0129 | ER-0041 | #177 | Scoped Type Registry |
| T-0130 | ER-0042 | #178 | Core Operations Metadata and Bindings |
| T-0131 | ER-0043 | #179 | CEO Runtime Hardening |
| T-0132 | ER-0044 | #180 | Capability Hooks and Policy Plumbing |
| T-0133 | ER-0045 | #181 | Kernel I/O Primitives Integration |
| T-0134 | ER-0045.1 | #181 | Conduit I/O Operations and Schema Definitions |
| T-0135 | ER-0045.2 | #181 | Conch Commands and Invocation Verification |
| T-0136 | ER-0045.3 | #181 | End-to-End I/O Integration Tests |
| T-0137 | ER-0045.4 | #181 | Persisted I/O Handles and Conch Aliases |
| T-0138 | ER-0046 | #182 | v1 Kernel Demo Integration and Roadmap |
| T-0139 | ER-0047 | #183 | Userland Core Utilities Suite |
| T-0140 | ER-0047.1 | #182 | Conch Session Operation Model |
| T-0141 | ER-0047.2 | #182 | Core Command Migration |
| T-0142 | ER-0047.3 | #182 | Utility Command Migration |
| T-0143 | ER-0047.4 | #182 | Command Alias Catalog |
| T-0144 | ER-0048 | #184 | Referee Import/Export Tools |
| T-0145 | ER-0049 | #185 | Object Graph Bundles and Packages |
| T-0146 | ER-0050 | #186 | Schema Migration Tools |
| T-0147 | ER-0051 | #187 | CEO Profiling and Trace Utilities |
| T-0148 | ER-0052 | #188 | Conch Debug and Inspection Tools |
| T-0149 | ER-0053 | #189 | v2 Integration and Usability Demo |
| T-0150 | ER-0054 | #184 | Conch Namespace Navigation |
| T-0151 | ER-0055 | #262 | Refract Inheritance and Interface Metadata |
| T-0152 | ER-0056 | #263 | Conch Typed AST and Shell Decomposition |
| T-0153 | ER-0057 | #264 | Service Host Lifecycle and Persistent Registry |
| T-0154 | ER-0058 | #265 | Capability Context Objects and Persistence |
| T-0155 | ER-0059 | #266 | CEO Task Capability Attachment |
| T-0156 | ER-0060 | #267 | Service Boundary Capability Enforcement |
| T-0157 | ER-0061 | #268 | Sandbox Identity and Service Isolation Hooks |
| T-0158 | ER-0062 | #269 | Memory Service Baseline |
| T-0159 | ER-0063 | #270 | Referee Graph Watch and Change-Feed API |
| T-0160 | ER-0064 | #271 | Vizier Relationship-Pattern Routing |
| T-0161 | ER-0065 | #272 | Task Visualization Objects and Task Conchos |
| T-0162 | ER-0066 | #273 | Observer-Driven Conch Session Growth |
| T-0163 | ER-0067 | #274 | Reusable Conch Grammar API |
| T-0164 | ER-0068 | #275 | Batch Execution and Non-Interactive Parser Integration |
| T-0165 | ER-0069 | #276 | Shared Parser Regression Harness |
| T-0166 | ER-0070 | #278 | Machine Representation Primitives |
| T-0167 | ER-0071 | #279 | Machine Descriptors and Resource Facts |
| T-0168 | ER-0072 | #280 | Machine Handles and Leases |
| T-0169 | ER-0073 | #281 | Comms Transport and Session Objects |
| T-0170 | ER-0074 | #282 | Comms Protocol Objects and Hardware Mapping |
| T-0171 | ER-0075 | #283 | Full Caliper Catalog Expansion |
| T-0172 | ER-0076 | #284 | Runtime Conversion and Compatibility Engine |
| T-0173 | ER-0077 | #285 | Conch Conversion and Inspection Commands |
| T-0174 | ER-0078 | #286 | Refract Constraints and Validation Metadata |
| T-0175 | ER-0079 | #287 | Refract Operation Effects Metadata |
| T-0176 | ER-0080 | #288 | Refract Documentation Objects and Introspection |
| I-0001 | DR-0001 | #116 | Conch Compare Alias Resolution |

## Validation Commands

The mapping used these local read-only commands:

```sh
rg -n "^# (ER|DR)-|^GitHub-Issue:" docs/ER docs/DR
ruby -e '...local Airframe ID to GitHub reference mapper...'
```

## Caveats

- Live GitHub issue state, title, and labels are not included.
- AR records are preserved separately and are not converted into Task or Issue mappings.
- Local duplicate GitHub references are preserved for review rather than normalized.
