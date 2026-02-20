#include "refract/bootstrap.h"

#include <array>
#include <cstdint>

namespace iris::refract {

namespace {

constexpr referee::TypeID kTypeString{0x1001ULL};
constexpr referee::TypeID kTypeU64{0x1002ULL};
constexpr referee::TypeID kTypeBool{0x1003ULL};
constexpr referee::TypeID kTypeObjectID{0x1004ULL};
constexpr referee::TypeID kTypeTypeID{0x1005ULL};
constexpr referee::TypeID kTypeVersion{0x1006ULL};
constexpr referee::TypeID kTypeBytes{0x1007ULL};

constexpr referee::TypeID kTypeFieldDefinition{0x5246524346000001ULL};
constexpr referee::TypeID kTypeOperationDefinition{0x5246524346000002ULL};
constexpr referee::TypeID kTypeSignatureDefinition{0x5246524346000003ULL};
constexpr referee::TypeID kTypeRelationshipSpec{0x5246524346000004ULL};

constexpr referee::TypeID kTypeRefereeObject{0x5245464500000001ULL};
constexpr referee::TypeID kTypeRefereeEdge{0x5245464500000002ULL};

constexpr referee::TypeID kTypeConchSession{0x434F4E4300000001ULL};
constexpr referee::TypeID kTypeConchConcho{0x434F4E4300000002ULL};

constexpr referee::TypeID kTypeVizPanel{0x56495A0000000001ULL};
constexpr referee::TypeID kTypeVizTextLog{0x56495A0000000002ULL};
constexpr referee::TypeID kTypeVizMetric{0x56495A0000000003ULL};
constexpr referee::TypeID kTypeVizTable{0x56495A0000000004ULL};
constexpr referee::TypeID kTypeVizTree{0x56495A0000000005ULL};

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

TypeDefinition make_referee_object() {
  TypeDefinition def;
  def.type_id = kTypeRefereeObject;
  def.name = "Object";
  def.namespace_name = "Referee";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "object_id", kTypeObjectID, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "version", kTypeVersion, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "type_id", kTypeTypeID, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "definition_id", kTypeObjectID, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "payload", kTypeBytes, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "created_at_ms", kTypeU64, true, std::nullopt });
  return def;
}

TypeDefinition make_referee_edge() {
  TypeDefinition def;
  def.type_id = kTypeRefereeEdge;
  def.name = "Edge";
  def.namespace_name = "Referee";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "from_id", kTypeObjectID, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "from_version", kTypeVersion, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "to_id", kTypeObjectID, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "to_version", kTypeVersion, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "role", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "props", kTypeBytes, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "created_at_ms", kTypeU64, true, std::nullopt });
  return def;
}

TypeDefinition make_conch_session() {
  TypeDefinition def;
  def.type_id = kTypeConchSession;
  def.name = "Session";
  def.namespace_name = "Conch";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "name", kTypeString, false, std::nullopt });
  return def;
}

TypeDefinition make_conch_concho() {
  TypeDefinition def;
  def.type_id = kTypeConchConcho;
  def.name = "Concho";
  def.namespace_name = "Conch";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "title", kTypeString, false, std::nullopt });
  return def;
}

TypeDefinition make_viz_panel() {
  TypeDefinition def;
  def.type_id = kTypeVizPanel;
  def.name = "Panel";
  def.namespace_name = "Viz";
  def.version = 1;
  return def;
}

TypeDefinition make_viz_text_log() {
  TypeDefinition def;
  def.type_id = kTypeVizTextLog;
  def.name = "TextLog";
  def.namespace_name = "Viz";
  def.version = 1;
  return def;
}

TypeDefinition make_viz_metric() {
  TypeDefinition def;
  def.type_id = kTypeVizMetric;
  def.name = "Metric";
  def.namespace_name = "Viz";
  def.version = 1;
  return def;
}

TypeDefinition make_viz_table() {
  TypeDefinition def;
  def.type_id = kTypeVizTable;
  def.name = "Table";
  def.namespace_name = "Viz";
  def.version = 1;
  return def;
}

TypeDefinition make_viz_tree() {
  TypeDefinition def;
  def.type_id = kTypeVizTree;
  def.name = "Tree";
  def.namespace_name = "Viz";
  def.version = 1;
  return def;
}

} // namespace

std::vector<TypeDefinition> core_schema_definitions() {
  std::vector<TypeDefinition> defs;
  defs.reserve(20);
  defs.push_back(make_primitive(kTypeString, "String"));
  defs.push_back(make_primitive(kTypeU64, "U64"));
  defs.push_back(make_primitive(kTypeBool, "Bool"));
  defs.push_back(make_primitive(kTypeObjectID, "ObjectID"));
  defs.push_back(make_primitive(kTypeTypeID, "TypeID"));
  defs.push_back(make_primitive(kTypeVersion, "Version"));
  defs.push_back(make_primitive(kTypeBytes, "Bytes"));
  defs.push_back(make_type_definition());
  defs.push_back(make_field_definition());
  defs.push_back(make_signature_definition());
  defs.push_back(make_operation_definition());
  defs.push_back(make_relationship_spec());
  defs.push_back(make_referee_object());
  defs.push_back(make_referee_edge());
  defs.push_back(make_conch_session());
  defs.push_back(make_conch_concho());
  defs.push_back(make_viz_panel());
  defs.push_back(make_viz_text_log());
  defs.push_back(make_viz_metric());
  defs.push_back(make_viz_table());
  defs.push_back(make_viz_tree());
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
