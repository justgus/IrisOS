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
