# AR implementation audit — 2026-10-20

- Request recovered from `docs/NOTE-FOR-CODEX.md`: reassess every AR and its ERs against the
  architecture, identifying work omitted by earlier completion claims.
- Baseline: `72674ec8289d38bc82766e0855ce2f163bed5e43`, main and origin/main after fetch.
- Coverage: all 27 canonical ARs and the scope/requirements/acceptance inventory of all 115 ERs,
  including ER-0045.1–0045.4 and ER-0047.1–0047.4.
- This is an implementation engineer's source/coverage audit with targeted counterexamples.
  It is not independent review, a new verification of every ER, or a human live pass.
- On 2026-10-20, the System Engineer directed the eleven ARs with concrete gaps to be marked
  `Pending Review`. Their canonical documents and architecture index now reflect that disposition.
  Architecture decisions, ER acceptance criteria, and existing verification evidence are preserved.

## Result

At the audit baseline, the recorded state was 27 Implemented ARs and 115 Verified ERs. That state
overstated the implemented architecture. After System Engineer disposition, 11 ARs are Pending
Review and 16 remain Implemented; the ER statuses are unchanged. Eleven ARs have concrete gaps
described below; six need their broader architecture
reconciled with narrower delivered scope. Ten have no additional direct gap identified in this
survey of their accepted scope. The latter assessment does not establish that those implementations
are defect-free or independent of the storage findings.

The recurring coverage problem is that an ER can correctly verify a schema declaration while its
parent AR also promises executable behavior. Verifying every listed ER does not prove the parent
AR when its breakdown omits requirements. Conversely, a missing runtime feature does not invalidate
a deliberately metadata-only ER. Both distinctions matter here.

## Every AR

All recorded statuses in this table are read from the canonical AR documents.

| AR | Title | Recorded status | Audit assessment and remaining work |
|---|---|---|---|
| AR-0001 | Expected-Style Error Model | Pending Review | Concrete gap, F1: public serialization helpers and ObjectID parsing throw across their API boundary. Existing Result/NotFound work remains present. |
| AR-0002 | Object Identity and Versioning Semantics | Pending Review | Concrete gap, F2: duplicate identity accepted; external form is 32 hex digits; cryptographic generation guarantee explicitly disclaimed. No dedicated identity ER covers these requirements. |
| AR-0003 | Storage Layout Strategy | Pending Review | Concrete gap, F3/F4: segment backend exists, but checksum, durable-write and torn-tail recovery requirements are unmet. V0 SQLite alignment text is stale relative to the implemented backend. |
| AR-0004 | Index and Graph Storage Strategy | Pending Review | Concrete recovery gap, F4. Rebuild exists, but successful recovery does not leave the store safe for subsequent append/reopen. Backend recommendation text also needs reconciliation. |
| AR-0005 | Service Plane Model | Implemented | Scope reconciliation, S1: bounded stages were accepted; registry is a C++ registry rather than a ServiceObject, and local dispatch does not establish the broader delivery/isolation model. Preserve ER-0057 acceptance. |
| AR-0006 | CEO/Exec Runtime Model | Implemented | Scope reconciliation, S2: task state/supervision, waitables and explicit reactor wake paths exist. No executable task scheduler/continuation, yield/sleep/timer surface is established by the registry. Preserve the 2026-10-14 acceptance of the established runtime. |
| AR-0007 | Refract Reflection Graph | Implemented | Scope reconciliation, S3: persisted inline metadata and accepted extended profiles exist; independently addressable Field/Signature/Operation/Documentation objects and variadic signature parameters are not demonstrated. |
| AR-0008 | Erector Subsystems | Implemented | Scope reconciliation, S4: Machine/Comms foundations exist; the larger primitive/descriptor/handle taxonomy is not delivered in full. Approved AR-0024 tracks remain distinct from the umbrella list. |
| AR-0009 | Referee Object Store | Pending Review | Concrete gap, F2/F5: creation always uses object revision 1; no API appends a new revision under a stable logical ObjectID. Definition supersession is a separate feature. |
| AR-0010 | Comms Subsystem | Implemented | Scope reconciliation, S4: loopback and protocol/mapping metadata exist; TCP/UDP/TLS/SSH, physical links, addressing and protocol implementations listed in this umbrella are not realized. |
| AR-0011 | Vizier Interpretation Layer | Implemented | No additional direct gap found in accepted routing/metadata scope. Type, relationship, task and emitted-artifact routing have source and named tests. |
| AR-0012 | Conch Shell and Concho Views | Implemented | No additional direct gap found in accepted textual session/workspace scope. Continuous idle notification and a graphical compositor are not inferred; preserve reported live passes. |
| AR-0013 | Viz Display Subsystem | Implemented | Scope reconciliation, S5: artifact creation/modeling exists; append-only live logs, live table mutation and metric history have no corresponding update/history API. Image is explicitly deferred. |
| AR-0014 | Conch Schema and Object Authoring | Pending Review | Concrete gap, F6: JSON schema authoring silently drops supplied operations. Existing field authoring/instance creation works. |
| AR-0015 | Conch Parser and Syntax | Implemented | No additional direct gap found in accepted single-line grammar, structured authoring, quoting and reusable parser scope. Full scripting remains excluded. |
| AR-0016 | Operations and Dispatch Model | Pending Review | Concrete gap, F7: shell call can report success without an implementation/result and resolves without argument types. Legacy API accepts inconsistent argument-type counts. |
| AR-0017 | Collections, Math Types, and Units | Pending Review | Concrete uncovered runtime work, F8: collection operation declarations and Astra shape metadata are present; usable collection bindings and math value/operation behavior are not. Metadata-only ERs do not cover this remainder. |
| AR-0018 | Generic Type System | Pending Review | Concrete gap, F9: arbitrary bases and missing fixed-arity arguments accepted; general parameter validation/substitution and Conch instance introspection missing; default hashing contradicts ER-0040. |
| AR-0019 | Caliper Unit Catalog | Implemented | No additional direct gap found in the explicitly approved finite catalog, rational conversion, extension and intrinsic-constraint scope. Do not reopen settled customary/prefix/temperature decisions. |
| AR-0020 | Core Type Operations and Rendering | Pending Review | Concrete gap, F10: most listed primitive operations and later collection/math phases are absent. U64 print/render produces blank output. Implemented contract add/to_string/compare is a narrower surface. |
| AR-0021 | Structured Types, Enums, and Packets | Pending Review | Concrete gap, F11: packet metadata lacks bit-layout serialization/validation; structured arrays and inherited layouts lack runtime coverage. Invalid packet definitions are accepted. |
| AR-0022 | Staged Service Plane Delivery | Implemented | No additional direct gap found against the explicitly approved bounded stages. Local allocator/network/inspection services do not imply remote networking or OS sandbox enforcement. |
| AR-0023 | Refract Reflection Profiles | Implemented | No additional direct gap found against approved metadata profiles. Inline documentation and required_capabilities were explicitly accepted; preserve that ruling. |
| AR-0024 | Erector Machine and Comms Delivery Tracks | Implemented | No additional direct gap found against approved foundational tracks and bounded loopback path. Hardware drivers/full stack remain excluded; preserve automated-evidence acceptance. |
| AR-0025 | Conch and Vizier Interaction Modes | Implemented | No additional direct gap found against accepted command-boundary snapshot/change-feed observer mode. Continuous idle notification is explicitly future work. |
| AR-0026 | Conch Parser Maturity Levels | Implemented | No additional direct gap found against documented command-family maturity levels. Shared grammar, typed authoring and batch surfaces exist. |
| AR-0027 | Standard Operation Contracts and Discovery | Implemented | No additional direct gap found against the approved initial three-contract plan. Vector kernels, full standard library and remote execution are expressly excluded. Final implementation CI now confirmed successful. |

## Concrete findings

### F1 — Public exceptions remain outside the Result boundary

`src/referee/referee.{h,cc}` exposes `ObjectID::from_hex`, `cbor_from_json_string` and
`json_string_from_cbor` as public throwing functions. The probe calls the JSON-to-CBOR helper with
malformed input and catches an exception outside the helper. Some consumers catch these exceptions;
that does not make the helper's public boundary conform to AR-0001. `ScopedTypeRegistry::set_logger`
also accepts a callback invoked without a catch in `log_promotion`; that path needs a separate
counterexample before claiming its exception behavior freshly reproduced.

ER-0086's audit inventory does not cover these helpers. Remaining work: enumerate every public
boundary, distinguish intentionally private throwing helpers, and cover typed decode/parse failures.

### F2 — Object identity uniqueness and representation

`SqliteStore::create_object_with_id` accepts an existing ObjectID and writes another revision-1
record. The probe creates payload 1 and then payload 2 under the same identity; both succeed and
latest lookup returns 2. This violates AR-0002's hard-error collision policy. Transaction-pending
duplicates need coverage too.

`ObjectID::to_hex` emits 32 hex digits; `from_hex` accepts that form rather than the canonical
hyphenated UUID text specified by AR-0002. `random` sets v4/variant bits but uses `std::random_device`;
the public header explicitly says cryptographic quality is not guaranteed. This is an unfulfilled
guarantee, not evidence that the current platform's entropy source is weak.

### F3 — Storage integrity and durable writes

`append_object`/`append_edge` write tags, lengths, metadata and bytes with no checksum. The probe
changes a payload byte on disk from 1 to 2; reopening and reading succeeds with 2. AR-0003 specifies
CRC integrity checks. Append functions return success after stream flush/index output without
checking the resulting stream state; no fsync/transaction commit marker establishes the promised
crash durability. `commit` appends pending records one at a time, with no atomic recovery record.
Power-loss durability and partial-commit consequences are source findings, not simulated crash
results in this run.

The backend uses `segments/` and `indexes/`, fixed object/edge files and full in-memory indexing.
That differs from AR-0003's versioned manifest/rolling segments and AR-0004's stale backend text.
The documented-equivalent allowance in ER-0034 requires a clear accepted format contract; it cannot
by itself waive integrity or recovery requirements.

### F4 — Torn-tail recovery leaves a latent failure

`load_segments` stops at an incomplete final record but does not truncate/quarantine the tail before
opening files for append. Probe sequence: write two records; truncate the last payload byte; reopen
successfully; write a third record successfully; close and reopen again. The final reopen fails
with `CorruptData` / `invalid object segment tag`. The earlier incomplete record consumes a byte
from the later append, misaligning the scan. This directly contradicts ER-0035's interrupted-write
recovery criterion. Existing persistence tests cover clean reopen and migration, not this sequence.

### F5 — Object revisions are not implemented

The public store API supports exact/latest reads, but its sole creation path assigns `Version{1}`.
There is no append-revision operation preserving ObjectID. Definition versions and supersedes
chains create different definition objects; they do not implement AR-0009's logical-object revision
history. Duplicate acceptance in F2 must not be used as a substitute for revision semantics.

### F6 — Optional operations disappear during Conch authoring

`parse_define_json_value` reads fields/relationships/packet/collection metadata but never reads an
`operations` member. The saved Conch input defines a type with an operation; `show type` reports its
field but no operation, without rejecting the unsupported member. AR-0014 explicitly includes
optional operations. ER-0021 verifies field authoring and persistence without covering this scope.
Remaining work: a supported operation authoring representation or a System Engineer scope decision.

### F7 — Legacy calls validate a declaration and report success without execution

`cmd_call` asks `DispatchEngine::resolve` with an empty argument-type vector, then handles a small
hardcoded primitive/demo set. All other matched declarations fall through to `call ok`. The saved
Conch transcript creates a Crate::List with arbitrary payload and invokes `size`; there is no size
result or implementation error. `DispatchEngine::resolve` also skips type matching when a nonempty
type vector's size differs from arg_count; the C++ probe reproduces acceptance of that mismatch.

ER-0030 only requires call validation, while AR-0016 describes invocation with typed dispatch.
ER-0042's executable core behavior is incomplete. AR-0027's ContractDispatcher fixes the new typed
contract path; its approved compatibility scope explicitly leaves the legacy path intact. Do not
claim the new API repaired every old call site.

### F8 — Collection and Astra metadata does not provide usable operations

`make_crate_*` and `make_astra_*` in `bootstrap.cc` register parameter labels and operations.
ER-0031/0032/0090/0093/0096/0097 explicitly cover metadata or shapes, not collection storage,
iteration/membership/index execution or math value operations. The library source list and binding
implementations do not supply those missing execution paths. F7 demonstrates the user-visible
consequence for size. Remaining work belongs in additional behavior ERs; do not relabel those
metadata ERs as runtime implementations or invent collection semantics under an audit.

### F9 — General generics stop short of the architecture

`GenericRegistry::register_instance` validates hardcoded Tuple/Vector/Matrix/Tensor forms, then
loads the GenericInstance schema rather than the requested base definition. The probe successfully
registers an unknown base and a Crate::List with no type argument. General definitions carry
`vector<string> type_params`, with no authoritative type/value/variadic kind or constraint model.
Substituting generic parameters into a general instantiated field/operation definition is absent.

`derive_generic_type_id` unconditionally hashes the canonical key with FNV-1a. The readable key
exists, but there is no scope/policy opt-in: ER-0040 explicitly rejects hashing by default.
Conch `show type` resolves TypeSummary definitions and displays parameter labels; it does not query
GenericRegistry for concrete instantiations or expose scope cache listings. ER-0094/0097 correctly
verify their limited built-in argument rules and do not settle the broader generic system.

### F10 — CoreOps coverage is much smaller than AR-0020

Refract::String exposes compare/print/render/to_string, rather than the listed equals, concat,
contains, starts_with, ends_with, slice, replace, case conversion and length operations. The larger
Bool/Bytes/U64 lists also lack most bindings; AR-0027 adds class-level U64/F64 add only. Generic
collection operations, shape-aware math rendering and generic signature integration phases have no
complete ER breakdown.

The saved transcript invokes `print` on U64 value 42 and receives a blank line followed by `call ok`:
`read_core_value` fills u64, but the print/render path always writes `CoreValue::text`. There is no
library-level print/render context binding or documented fallback for arbitrary instances/types.
ER-0042 needs reopened behavior coverage; later AR-0020 phases need explicit ERs and scope decisions.

### F11 — Structured layout metadata has no corresponding binary implementation

PacketFieldDefinition stores name/type/bit_width; registration and schema round trips preserve it.
The probe registers a zero-width field and invalid byte-order string successfully. Conch instance
creation validates Required/NonEmpty constraints, then serializes an ordinary CBOR object; it never
uses packet bit offsets, widths or byte order. The transcript also shows a String accepted for a
declared U64 field. Required presence is correctly enforced; type/layout validation is not.

No Refract packet encoder/decoder, nested packet layout resolver, packet-array element serializer or
inherited structured-field validator is established by ER-0038/0039's tests. Machine::Packet is a
separate byte container and is not an implementation of a declared Refract packet layout.
ER-0039 explicitly requires deterministic packet-array element layouts, so this is an ER coverage
failure as well as an uncovered AR requirement. ER-0038 remains evidence for metadata only.

## Scope reconciliations requiring System Engineer decisions

| Item | Existing evidence | Unresolved boundary |
|---|---|---|
| S1 | ServiceRegistry, synchronous IpcService, lifecycle manager and accepted bounded Stage E | Whether AR-0005 remains a long-term umbrella; registry as a ServiceObject, ACK/retry delivery semantics and enforced isolation are not established by current stages. Timeout is checked after the synchronous handler returns. |
| S2 | TaskRegistry state transitions, WaitResult waiter IDs, explicit resume/reactor operations and accepted runtime | Whether executable scheduling/continuations, yield/sleep, timers and runtime ownership hooks are deferred. The accepted state-machine substrate is real; no runnable scheduler is inferred from a Running enum. |
| S3 | TypeDefinition contains nested reflection metadata; inline documentation and capability metadata accepted in AR-0023 | Whether AR-0007's independently addressable/versioned graph entities and variadic signatures remain required beyond the approved metadata profiles. |
| S4 | Approved Machine primitives/descriptors/leases and loopback protocol compatibility tracks | How to track omitted Pointer/checksum/hash, NUMA/cache/TLB/DMA and concrete physical-link/transport/protocol taxonomy from AR-0008/0010. Complete drivers/full network stacks remain explicitly excluded from AR-0024. |
| S5 | create_text_log/create_table/create_metric persist initial artifact snapshots | Whether AR-0013's live logs/tables/metrics require update/history APIs or are intended to mean immutable snapshots. Image remains an explicit later feature. |

No new broad implementation or acceptance-criteria rewrite is authorized by these findings. The
System Engineer returned the eleven concrete-gap ARs to Pending Review. Reassessment of
ER-0034/0035/0039/0040/0042/0086 remains a recommendation; no ER status was changed. Other ERs
can remain valid for their narrower scope; missing parent requirements need new ERs.
Decide the umbrella boundaries separately instead of silently shrinking them to match current code.

## Reproduction and validation

The executable probe and shell input/output are in [the audit artifacts](audits/2026-10-20/).
The full [ER inventory](audits/2026-10-20/ER-inventory.md) records each ER's canonical and ledger
status, linked parents, existing source/test evidence and assessment. It is not a fresh 115-ER pass.

Commands run from the repository root:

```sh
git fetch origin main
git rev-parse HEAD origin/main
make -q -C src libreferee.la
g++ -std=gnu++20 -Isrc docs/Plans/audits/2026-10-20/probe.cc -Lsrc/.libs -lreferee -o .ar-audit-probe
env DYLD_LIBRARY_PATH="$PWD/src/.libs" ./.ar-audit-probe .ar-audit-new-store
env DYLD_LIBRARY_PATH="$PWD/src/.libs" ./bin/conch --db :memory: < docs/Plans/audits/2026-10-20/conch-input.txt
git diff --check
```

`make -q` returned 0: the existing library is current for the audited source. The actual initial
compile also included `-Wl,-rpath,/Users/justgus/Xcode-Projects/IrisOS/src/.libs`; it compiled
successfully, but the initial run without DYLD_LIBRARY_PATH failed at the dylib loader before any
probe code executed. The environment-qualified rerun completed with exit 0 and reproduced the
findings. The probe requires a nonexistent disposable directory and modifies only files it creates.
Its exit code means the audit scenarios ran, not that architecture acceptance passed.

No production source, build inputs or generated artifacts changed; no Autotools regeneration or
full local suite was run for this audit. Existing staged libtool/test_referee_core changes and the
user's handoff note were preserved. The saved shell output contains nondeterministic object IDs.

Previously pending implementation CI [39052974745](https://github.com/justgus/IrisOS/actions/runs/39052974745)
was read back as completed/success for `278ae380e3c066cbd7a5b8449e628939b0f787b2`, including build,
unit tests and distcheck. This is baseline regression evidence, not coverage of the new findings.
