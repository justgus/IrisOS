#pragma once

#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace iris::refract {

struct FieldDefinition {
  std::string name;
  referee::TypeID type{};
  bool required{false};
  std::optional<std::string> default_json;
};

struct ParameterDefinition {
  std::string name;
  referee::TypeID type{};
  bool optional{false};
};

struct SignatureDefinition {
  std::vector<ParameterDefinition> params;
  std::vector<ParameterDefinition> outputs;
};

enum class OperationScope {
  Class,
  Object
};

struct OperationDefinition {
  std::string name;
  OperationScope scope{OperationScope::Object};
  SignatureDefinition signature;
};

struct RelationshipSpec {
  std::string role;
  std::string cardinality;
  std::string target;
};

struct TypeDefinition {
  referee::TypeID type_id{};
  std::string name;
  std::string namespace_name;
  std::uint64_t version{1};
  std::optional<referee::ObjectID> supersedes_definition_id{};
  std::optional<std::string> migration_hook{};
  std::vector<std::string> type_params;
  std::vector<FieldDefinition> fields;
  std::vector<OperationDefinition> operations;
  std::vector<RelationshipSpec> relationships;
  std::optional<std::string> preferred_renderer{};
};

struct TypeSummary {
  referee::TypeID type_id{};
  referee::ObjectID definition_id{};
  std::string name;
  std::string namespace_name;
  std::optional<std::string> preferred_renderer;
};

struct DefinitionRecord {
  referee::ObjectRef ref{};
  TypeDefinition definition{};
};

struct SupersedesLink {
  DefinitionRecord prior{};
  std::optional<std::string> migration_hook;
};

constexpr referee::TypeID kTypeDefinitionType{0x5246524354450001ULL};

class SchemaRegistry {
public:
  explicit SchemaRegistry(referee::SqliteStore& store);

  referee::Result<DefinitionRecord> register_definition(const TypeDefinition& def);
  referee::Result<DefinitionRecord> register_definition_with_id(const TypeDefinition& def,
                                                                referee::ObjectID definition_id);
  referee::Result<std::optional<DefinitionRecord>> get_definition_by_id(referee::ObjectID id);
  referee::Result<std::optional<DefinitionRecord>> get_definition_by_type(referee::TypeID type);
  referee::Result<std::optional<DefinitionRecord>> get_latest_definition_by_type(referee::TypeID type);
  referee::Result<std::vector<TypeSummary>> list_types();
  referee::Result<std::vector<SupersedesLink>> list_supersedes_chain(referee::ObjectID definition_id);

private:
  referee::SqliteStore& store_;
};

} // namespace iris::refract
