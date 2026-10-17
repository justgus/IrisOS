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
