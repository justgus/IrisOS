#include "refract/bootstrap.h"

#include <array>
#include <cstdint>

namespace iris::refract {

namespace {

constexpr referee::TypeID kTypeString{0x1001ULL};
constexpr referee::TypeID kTypeU64{0x1002ULL};
constexpr referee::TypeID kTypeBool{0x1003ULL};

constexpr referee::TypeID kTypeFieldDefinition{0x5246524346000001ULL};
constexpr referee::TypeID kTypeOperationDefinition{0x5246524346000002ULL};
constexpr referee::TypeID kTypeSignatureDefinition{0x5246524346000003ULL};
constexpr referee::TypeID kTypeRelationshipSpec{0x5246524346000004ULL};

referee::ObjectID definition_id_for(referee::TypeID type_id) {
  referee::ObjectID id{};
  std::array<std::uint8_t, 8> tag = { 'R','E','F','R','A','C','T','0' };
  for (std::size_t i = 0; i < tag.size(); ++i) id.bytes[i] = tag[i];

  std::uint64_t v = type_id.v;
  for (int i = 0; i < 8; ++i) {
    id.bytes[15 - i] = (std::uint8_t)(v & 0xFFu);
    v >>= 8;
  }
  return id;
}

TypeDefinition make_primitive(referee::TypeID type_id, const std::string& name) {
  TypeDefinition def;
  def.type_id = type_id;
  def.name = name;
  def.namespace_name = "Refract";
  def.version = 1;
  return def;
}

TypeDefinition make_type_definition() {
  TypeDefinition def;
  def.type_id = kTypeDefinitionType;
  def.name = "TypeDefinition";
  def.namespace_name = "Refract";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "type_id", kTypeU64, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "namespace", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "version", kTypeU64, true, std::nullopt });
  return def;
}

TypeDefinition make_field_definition() {
  TypeDefinition def;
  def.type_id = kTypeFieldDefinition;
  def.name = "FieldDefinition";
  def.namespace_name = "Refract";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "type_id", kTypeU64, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "required", kTypeBool, false, std::nullopt });
  return def;
}

TypeDefinition make_signature_definition() {
  TypeDefinition def;
  def.type_id = kTypeSignatureDefinition;
  def.name = "SignatureDefinition";
  def.namespace_name = "Refract";
  def.version = 1;
  return def;
}

TypeDefinition make_operation_definition() {
  TypeDefinition def;
  def.type_id = kTypeOperationDefinition;
  def.name = "OperationDefinition";
  def.namespace_name = "Refract";
  def.version = 1;
  return def;
}

TypeDefinition make_relationship_spec() {
  TypeDefinition def;
  def.type_id = kTypeRelationshipSpec;
  def.name = "RelationshipSpec";
  def.namespace_name = "Refract";
  def.version = 1;
  return def;
}

} // namespace

std::vector<TypeDefinition> core_schema_definitions() {
  std::vector<TypeDefinition> defs;
  defs.reserve(8);
  defs.push_back(make_primitive(kTypeString, "String"));
  defs.push_back(make_primitive(kTypeU64, "U64"));
  defs.push_back(make_primitive(kTypeBool, "Bool"));
  defs.push_back(make_type_definition());
  defs.push_back(make_field_definition());
  defs.push_back(make_signature_definition());
  defs.push_back(make_operation_definition());
  defs.push_back(make_relationship_spec());
  return defs;
}

referee::Result<BootstrapResult> bootstrap_core_schema(SchemaRegistry& registry) {
  BootstrapResult out;
  auto defs = core_schema_definitions();

  for (const auto& def : defs) {
    auto existing = registry.get_definition_by_type(def.type_id);
    if (!existing) return referee::Result<BootstrapResult>::err(existing.error->message);
    if (existing.value->has_value()) {
      ++out.existing;
      continue;
    }
    auto definition_id = definition_id_for(def.type_id);
    auto reg = registry.register_definition_with_id(def, definition_id);
    if (!reg) return referee::Result<BootstrapResult>::err(reg.error->message);
    ++out.inserted;
  }

  return referee::Result<BootstrapResult>::ok(out);
}

} // namespace iris::refract
