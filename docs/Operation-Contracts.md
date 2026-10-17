# Operation Contracts

Refract's OperationCatalog stores immutable contracts keyed by a 128-bit ObjectID and a positive
integer version. Display names are not identities. Registering identical metadata again is
idempotent; changing metadata under an existing key returns AlreadyExists. New versions remain
explicitly addressable alongside old versions.

A contract declares its scope, category, ordered input/output parameters, behavior documentation,
effects and required capabilities. A parameter contains either a concrete TypeID or a named type
parameter, never both. Optional parameters must trail required ones. Generic arguments are
represented by the existing concrete GenericRegistry instance TypeIDs.

OperationCatalog::list supports optional category and required-capability filters and returns
identity/version order. get requires the exact version. Malformed persisted catalog records fail
with CorruptData, including when a filter would otherwise exclude that record. Metadata describes
requirements; it does not grant authority.

Catalog calls are serialized, following SchemaRegistry's existing ownership model. The backing
SqliteStore must outlive the catalog. Contracts use ordinary Referee objects, not a separate
network discovery protocol. The catalog API preserves typed storage errors.

Types advertise support through OperationDefinition::conformances, including an exact contract key
and explicit substitutions for every contract type parameter. Self must name the declaring type.
Registration validates scope, ordered concrete signature and optionality, required capabilities,
and contract effects before storing a definition. Compatible names alone never establish support.
Existing operations without conformance retain their previous encoding and behavior.

Each conforming operation also has an implementation ObjectID/version and execution route.
Local routes have no destination; Service and ObjectSystem routes require a destination string.
OperationRegistry::list_conformances preserves declaring owner and inheritance depth; the object
view loads the object's stored type. Discovery revalidates claims against the catalog and fails on
corrupt metadata. It does not issue capabilities or create per-object authority overrides.

ContractDispatcher resolves an exact contract key using every typed input. It follows existing
exact/subtype, omitted-optional-argument, and declaring-depth precedence; tied candidates return
FailedPrecondition with an ambiguity diagnostic. It takes an input vector, so a separate untrusted
argument count cannot bypass type checks. No implicit conversion or generic inference occurs.

Invoke checks object/class receiver shape, required capabilities and execution route before a
registered callback runs. Missing callbacks require explicit runtime rebinding; delegated routes
return Unsupported in this local implementation. Callback errors propagate and exceptions become
Internal errors. Returned values must match declared output count/types; results never imply
assignment to a caller-provided destination. Callbacks remain responsible for their documented
payload semantics and declared effects. Type tags are trusted local runtime metadata, not a new
remote authentication mechanism.

## Initial standard catalog

| Contract | Identity (version 1) | Scope | Concrete implementations |
|---|---|---|---|
| to_string | 27000000000000000000000000000001 | Object | String, U64, Bool, ObjectID, TypeID, Version, Bytes, F64 |
| compare | 27000000000000000000000000000002 | Object | Same primitive set; returns F64 -1, 0, or 1 |
| add | 27000000000000000000000000000003 | Class | U64 and F64; two inputs and a separate result |

Each contract uses an explicit Self substitution. U64 addition rejects overflow. F64 callbacks
require finite values, and addition rejects non-finite results. Primitive payloads use CBOR objects
with a `value` member. Bytes accepts strings or arrays of unsigned octets; array display uses
lowercase hexadecimal, following the existing primitive convention. New callbacks reject malformed payloads with InvalidArgument. Existing
name-based Conch calls retain their established execution path and behavior.

After bootstrap_core_schema, call bootstrap_standard_contracts(schema, catalog) to register the
catalog and adopt primitive operations in new schema versions. Prior definition records remain
available. Repeated adoption is idempotent; a conflicting existing operation or binding fails
explicitly. This is a serialized bootstrap; if interrupted, retry completes remaining adoptions.

Construct a ContractDispatcher with the schema and catalog, then call bind_standard_contracts to
bind the compiled primitive implementations. A fresh dispatcher has no callbacks, including after
reopen. invoke receives the target TypeID, exact ContractKey, typed CBOR inputs, and caller
capabilities. Object operations also receive a typed receiver; invoke_object loads it by ObjectID
from the store. Invocation returns typed values; assigning a returned result is a separate caller
action. Effects describe behavior and never supply a missing capability.

For example, add U64 values through the C++ API:

```cpp
using namespace iris::refract;
ContractValues inputs{
    {{0x1002}, nlohmann::json::to_cbor({{"value", 4}})},
    {{0x1002}, nlohmann::json::to_cbor({{"value", 5}})}};
auto result = dispatcher.invoke({0x1002}, standard_contract_key(StandardContract::Add), inputs, {});
```

## Conch discovery

Conch adopts the standard catalog at startup. These inspection commands use the canonical Refract
metadata and print deterministic JSON views:

```text
ops --catalog
ops --catalog --category arithmetic
ops --catalog --capability math.use
ops Refract::U64 --contracts
```

The catalog view contains identity/version, signature, category, behavior, effects, and required
capabilities. The type view also includes concrete inputs/outputs, substitutions, declaring owner,
inheritance depth and implementation route. Empty filters return an empty array. Programmatic
OperationCatalog::get selects exact identity/version; object discovery is available through
OperationRegistry::list_object_conformances. Local runtime callback availability is checked at
invocation, not inferred from persisted route metadata. No remote transport or vector/matrix
kernel is provided by this initial catalog.
