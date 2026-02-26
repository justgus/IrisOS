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
constexpr referee::TypeID kTypeF64{0x1008ULL};

constexpr referee::TypeID kTypeCrateArray{0x4352415400000001ULL};
constexpr referee::TypeID kTypeCrateList{0x4352415400000002ULL};
constexpr referee::TypeID kTypeCrateSet{0x4352415400000003ULL};
constexpr referee::TypeID kTypeCrateMap{0x4352415400000004ULL};
constexpr referee::TypeID kTypeCrateTuple{0x4352415400000005ULL};

constexpr referee::TypeID kTypeAstraFloat{0x4153545200000001ULL};
constexpr referee::TypeID kTypeAstraDouble{0x4153545200000002ULL};
constexpr referee::TypeID kTypeAstraVector{0x4153545200000003ULL};
constexpr referee::TypeID kTypeAstraMatrix{0x4153545200000004ULL};
constexpr referee::TypeID kTypeAstraTensor{0x4153545200000005ULL};

constexpr referee::TypeID kTypeFieldDefinition{0x5246524346000001ULL};
constexpr referee::TypeID kTypeOperationDefinition{0x5246524346000002ULL};
constexpr referee::TypeID kTypeSignatureDefinition{0x5246524346000003ULL};
constexpr referee::TypeID kTypeRelationshipSpec{0x5246524346000004ULL};

constexpr referee::TypeID kTypeRefereeObject{0x5245464500000001ULL};
constexpr referee::TypeID kTypeRefereeEdge{0x5245464500000002ULL};

constexpr referee::TypeID kTypeConchSession{0x434F4E4300000001ULL};
constexpr referee::TypeID kTypeConchConcho{0x434F4E4300000002ULL};
constexpr referee::TypeID kTypeConchAlias{0x434F4E4300000003ULL};

constexpr referee::TypeID kTypeVizPanel{0x56495A0000000001ULL};
constexpr referee::TypeID kTypeVizTextLog{0x56495A0000000002ULL};
constexpr referee::TypeID kTypeVizMetric{0x56495A0000000003ULL};
constexpr referee::TypeID kTypeVizTable{0x56495A0000000004ULL};
constexpr referee::TypeID kTypeVizTree{0x56495A0000000005ULL};

constexpr referee::TypeID kTypeDemoPropulsionSynth{0x44454D4F00000001ULL};
constexpr referee::TypeID kTypeDemoSummary{0x44454D4F00000002ULL};
constexpr referee::TypeID kTypeDemoDetail{0x44454D4F00000003ULL};

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

void add_size_operation(TypeDefinition& def) {
  OperationDefinition op;
  op.name = "size";
  op.scope = OperationScope::Object;
  op.signature.outputs.push_back(ParameterDefinition{ "count", kTypeU64, false });
  def.operations.push_back(std::move(op));
}

void add_iterate_operation(TypeDefinition& def) {
  OperationDefinition op;
  op.name = "iterate";
  op.scope = OperationScope::Object;
  op.signature.outputs.push_back(ParameterDefinition{ "items", kTypeBytes, false });
  def.operations.push_back(std::move(op));
}

void add_index_operation(TypeDefinition& def, referee::TypeID index_type, referee::TypeID value_type) {
  OperationDefinition op;
  op.name = "index";
  op.scope = OperationScope::Object;
  op.signature.params.push_back(ParameterDefinition{ "index", index_type, false });
  op.signature.outputs.push_back(ParameterDefinition{ "value", value_type, false });
  def.operations.push_back(std::move(op));
}

void add_contains_operation(TypeDefinition& def, referee::TypeID value_type) {
  OperationDefinition op;
  op.name = "contains";
  op.scope = OperationScope::Object;
  op.signature.params.push_back(ParameterDefinition{ "value", value_type, false });
  op.signature.outputs.push_back(ParameterDefinition{ "present", kTypeBool, false });
  def.operations.push_back(std::move(op));
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

TypeDefinition make_bytes_definition() {
  TypeDefinition def = make_primitive(kTypeBytes, "Bytes");
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeU64);
  add_contains_operation(def, kTypeU64);
  return def;
}

TypeDefinition make_crate_array() {
  TypeDefinition def;
  def.type_id = kTypeCrateArray;
  def.name = "Array";
  def.namespace_name = "Crate";
  def.version = 1;
  def.type_params = { "T" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_crate_list() {
  TypeDefinition def;
  def.type_id = kTypeCrateList;
  def.name = "List";
  def.namespace_name = "Crate";
  def.version = 1;
  def.type_params = { "T" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_crate_set() {
  TypeDefinition def;
  def.type_id = kTypeCrateSet;
  def.name = "Set";
  def.namespace_name = "Crate";
  def.version = 1;
  def.type_params = { "T" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_crate_map() {
  TypeDefinition def;
  def.type_id = kTypeCrateMap;
  def.name = "Map";
  def.namespace_name = "Crate";
  def.version = 1;
  def.type_params = { "K", "V" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeBytes, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_crate_tuple() {
  TypeDefinition def;
  def.type_id = kTypeCrateTuple;
  def.name = "Tuple";
  def.namespace_name = "Crate";
  def.version = 1;
  def.type_params = { "Ts" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_astra_float() {
  TypeDefinition def;
  def.type_id = kTypeAstraFloat;
  def.name = "Float";
  def.namespace_name = "Astra";
  def.version = 1;
  return def;
}

TypeDefinition make_astra_double() {
  TypeDefinition def;
  def.type_id = kTypeAstraDouble;
  def.name = "Double";
  def.namespace_name = "Astra";
  def.version = 1;
  return def;
}

TypeDefinition make_astra_vector() {
  TypeDefinition def;
  def.type_id = kTypeAstraVector;
  def.name = "Vector";
  def.namespace_name = "Astra";
  def.version = 1;
  def.type_params = { "T", "N" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_astra_matrix() {
  TypeDefinition def;
  def.type_id = kTypeAstraMatrix;
  def.name = "Matrix";
  def.namespace_name = "Astra";
  def.version = 1;
  def.type_params = { "T", "R", "C" };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
  return def;
}

TypeDefinition make_astra_tensor() {
  TypeDefinition def;
  def.type_id = kTypeAstraTensor;
  def.name = "Tensor";
  def.namespace_name = "Astra";
  def.version = 1;
  def.type_params = { "T", "Dims..." };
  add_size_operation(def);
  add_iterate_operation(def);
  add_index_operation(def, kTypeU64, kTypeBytes);
  add_contains_operation(def, kTypeBytes);
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
  def.fields.push_back(FieldDefinition{ "params", kTypeBytes, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "outputs", kTypeBytes, false, std::nullopt });
  return def;
}

TypeDefinition make_operation_definition() {
  TypeDefinition def;
  def.type_id = kTypeOperationDefinition;
  def.name = "OperationDefinition";
  def.namespace_name = "Refract";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "scope", kTypeString, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "signature", kTypeBytes, false, std::nullopt });
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

TypeDefinition make_conch_alias() {
  TypeDefinition def;
  def.type_id = kTypeConchAlias;
  def.name = "Alias";
  def.namespace_name = "Conch";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "object_id", kTypeObjectID, true, std::nullopt });
  return def;
}

TypeDefinition make_viz_panel() {
  TypeDefinition def;
  def.type_id = kTypeVizPanel;
  def.name = "Panel";
  def.namespace_name = "Viz";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "title", kTypeString, false, std::nullopt });
  return def;
}

TypeDefinition make_viz_text_log() {
  TypeDefinition def;
  def.type_id = kTypeVizTextLog;
  def.name = "TextLog";
  def.namespace_name = "Viz";
  def.version = 1;
  def.preferred_renderer = "Log";
  def.fields.push_back(FieldDefinition{ "lines", kTypeBytes, false, std::nullopt });
  return def;
}

TypeDefinition make_viz_metric() {
  TypeDefinition def;
  def.type_id = kTypeVizMetric;
  def.name = "Metric";
  def.namespace_name = "Viz";
  def.version = 1;
  def.preferred_renderer = "Metric";
  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "value", kTypeF64, true, std::nullopt });
  return def;
}

TypeDefinition make_viz_table() {
  TypeDefinition def;
  def.type_id = kTypeVizTable;
  def.name = "Table";
  def.namespace_name = "Viz";
  def.version = 1;
  def.preferred_renderer = "Table";
  def.fields.push_back(FieldDefinition{ "columns", kTypeBytes, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "rows", kTypeBytes, false, std::nullopt });
  return def;
}

TypeDefinition make_viz_tree() {
  TypeDefinition def;
  def.type_id = kTypeVizTree;
  def.name = "Tree";
  def.namespace_name = "Viz";
  def.version = 1;
  def.preferred_renderer = "Tree";
  def.fields.push_back(FieldDefinition{ "label", kTypeString, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "children", kTypeBytes, false, std::nullopt });
  return def;
}

TypeDefinition make_demo_propulsion_synth() {
  TypeDefinition def;
  def.type_id = kTypeDemoPropulsionSynth;
  def.name = "PropulsionSynth";
  def.namespace_name = "Demo";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "name", kTypeString, false, std::nullopt });

  OperationDefinition start_op;
  start_op.name = "start";
  start_op.scope = OperationScope::Object;
  def.operations.push_back(std::move(start_op));

  def.relationships.push_back(RelationshipSpec{ "summary", "one", "Demo::Summary" });
  return def;
}

TypeDefinition make_demo_summary() {
  TypeDefinition def;
  def.type_id = kTypeDemoSummary;
  def.name = "Summary";
  def.namespace_name = "Demo";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "title", kTypeString, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "level", kTypeU64, false, std::nullopt });

  OperationDefinition expand_op;
  expand_op.name = "expand";
  expand_op.scope = OperationScope::Object;
  expand_op.signature.params.push_back(ParameterDefinition{ "level", kTypeU64, true });
  def.operations.push_back(std::move(expand_op));

  def.relationships.push_back(RelationshipSpec{ "detail", "many", "Demo::Detail" });
  return def;
}

TypeDefinition make_demo_detail() {
  TypeDefinition def;
  def.type_id = kTypeDemoDetail;
  def.name = "Detail";
  def.namespace_name = "Demo";
  def.version = 1;

  def.fields.push_back(FieldDefinition{ "title", kTypeString, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "level", kTypeU64, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "index", kTypeU64, false, std::nullopt });
  return def;
}

} // namespace

std::vector<TypeDefinition> core_schema_definitions() {
  std::vector<TypeDefinition> defs;
  defs.reserve(36);
  defs.push_back(make_primitive(kTypeString, "String"));
  defs.push_back(make_primitive(kTypeU64, "U64"));
  defs.push_back(make_primitive(kTypeBool, "Bool"));
  defs.push_back(make_primitive(kTypeObjectID, "ObjectID"));
  defs.push_back(make_primitive(kTypeTypeID, "TypeID"));
  defs.push_back(make_primitive(kTypeVersion, "Version"));
  defs.push_back(make_bytes_definition());
  defs.push_back(make_primitive(kTypeF64, "F64"));
  defs.push_back(make_crate_array());
  defs.push_back(make_crate_list());
  defs.push_back(make_crate_set());
  defs.push_back(make_crate_map());
  defs.push_back(make_crate_tuple());
  defs.push_back(make_astra_float());
  defs.push_back(make_astra_double());
  defs.push_back(make_astra_vector());
  defs.push_back(make_astra_matrix());
  defs.push_back(make_astra_tensor());
  defs.push_back(make_type_definition());
  defs.push_back(make_field_definition());
  defs.push_back(make_signature_definition());
  defs.push_back(make_operation_definition());
  defs.push_back(make_relationship_spec());
  defs.push_back(make_referee_object());
  defs.push_back(make_referee_edge());
  defs.push_back(make_conch_session());
  defs.push_back(make_conch_concho());
  defs.push_back(make_conch_alias());
  defs.push_back(make_viz_panel());
  defs.push_back(make_viz_text_log());
  defs.push_back(make_viz_metric());
  defs.push_back(make_viz_table());
  defs.push_back(make_viz_tree());
  defs.push_back(make_demo_propulsion_synth());
  defs.push_back(make_demo_summary());
  defs.push_back(make_demo_detail());
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
