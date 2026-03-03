#pragma once

#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdint>
#include <functional>
#include <map>
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

struct EnumValueDefinition {
  std::string name{};
  std::string value_json{};
};

struct PacketFieldDefinition {
  std::string name{};
  referee::TypeID type{};
  std::uint32_t bit_width{0};
};

struct CollectionElementDefinition {
  std::string role{};
  referee::TypeID type{};
};

enum class GenericArgKind {
  Type,
  Value,
  Variadic
};

struct GenericArg {
  GenericArgKind kind{GenericArgKind::Type};
  referee::TypeID type_id{};
  referee::TypeID value_type{};
  std::string value_json{};
  std::vector<GenericArg> items{};
};

struct GenericInstance {
  referee::TypeID base_type{};
  std::vector<GenericArg> args{};
  std::optional<std::string> display;
  referee::TypeID instance_type{};
};

struct TypeDefinition {
  referee::TypeID type_id{};
  std::string name{};
  std::string namespace_name{};
  std::uint64_t version{1};
  std::optional<std::string> kind{};
  std::optional<referee::ObjectID> supersedes_definition_id{};
  std::optional<std::string> migration_hook{};
  std::vector<std::string> type_params{};
  std::vector<FieldDefinition> fields{};
  bool has_enum_value_type{false};
  referee::TypeID enum_value_type{};
  std::vector<EnumValueDefinition> enum_values{};
  std::optional<std::string> packet_byte_order{};
  std::vector<PacketFieldDefinition> packet_fields{};
  std::optional<std::string> collection_kind{};
  std::vector<CollectionElementDefinition> collection_elements{};
  std::vector<OperationDefinition> operations{};
  std::vector<RelationshipSpec> relationships{};
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
constexpr referee::TypeID kTypeGenericInstanceType{0x5246524347000001ULL};

referee::Result<std::string> encode_generic_instance_key(const GenericInstance& instance);
referee::Result<referee::TypeID> derive_generic_type_id(const GenericInstance& instance);

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

struct GenericInstanceRecord {
  referee::ObjectRef ref{};
  GenericInstance instance{};
};

class GenericRegistry {
public:
  GenericRegistry(SchemaRegistry& schema, referee::SqliteStore& store);

  referee::Result<GenericInstanceRecord> register_instance(const GenericInstance& instance);
  referee::Result<std::optional<GenericInstanceRecord>> get_instance_by_type(referee::TypeID type_id);

private:
  SchemaRegistry& schema_;
  referee::SqliteStore& store_;
};

class ScopedTypeRegistry {
public:
  enum class Scope {
    Operation,
    Application,
    Database,
    Sandbox,
    Global
  };

  enum class PromotionPolicy {
    LocalOnly,
    Parent,
    Root
  };

  ScopedTypeRegistry(Scope scope, GenericRegistry& registry, ScopedTypeRegistry* parent = nullptr);

  void set_logger(std::function<void(const std::string&)> logger);

  referee::Result<GenericInstanceRecord> resolve_or_register(
      const GenericInstance& instance,
      PromotionPolicy policy);

  referee::Result<std::optional<GenericInstanceRecord>> find(referee::TypeID type_id);
  std::optional<GenericInstanceRecord> find_local(referee::TypeID type_id) const;

  void cache_instance(const GenericInstanceRecord& record);

private:
  struct TypeIDLess {
    bool operator()(referee::TypeID a, referee::TypeID b) const { return a.v < b.v; }
  };

  Scope scope_;
  GenericRegistry& registry_;
  ScopedTypeRegistry* parent_{nullptr};
  std::map<referee::TypeID, GenericInstanceRecord, TypeIDLess> cache_;
  std::function<void(const std::string&)> logger_;

  ScopedTypeRegistry* root();
  void promote_to(ScopedTypeRegistry* target, const GenericInstanceRecord& record);
  void log_promotion(Scope from, Scope to, referee::TypeID type_id);
};

} // namespace iris::refract
