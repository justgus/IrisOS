#include "refract/bootstrap.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>

#include <nlohmann/json.hpp>

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

constexpr referee::TypeID kTypeCaliperDimension{0x43414C5000000001ULL};
constexpr referee::TypeID kTypeCaliperUnit{0x43414C5000000002ULL};
constexpr referee::TypeID kTypeCaliperAngle{0x43414C5000000003ULL};
constexpr referee::TypeID kTypeCaliperDuration{0x43414C5000000004ULL};
constexpr referee::TypeID kTypeCaliperSpan{0x43414C5000000005ULL};
constexpr referee::TypeID kTypeCaliperRange{0x43414C5000000006ULL};
constexpr referee::TypeID kTypeCaliperPercentage{0x43414C5000000007ULL};
constexpr referee::TypeID kTypeCaliperRatio{0x43414C5000000008ULL};

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

void add_compatible_operation(TypeDefinition& def, referee::TypeID other_type) {
  OperationDefinition op;
  op.name = "compatible";
  op.scope = OperationScope::Object;
  op.signature.params.push_back(ParameterDefinition{ "other", other_type, false });
  op.signature.outputs.push_back(ParameterDefinition{ "compatible", kTypeBool, false });
  def.operations.push_back(std::move(op));
}

void add_convert_operation(TypeDefinition& def, referee::TypeID value_type, referee::TypeID unit_type) {
  OperationDefinition op;
  op.name = "convert";
  op.scope = OperationScope::Object;
  op.signature.params.push_back(ParameterDefinition{ "value", value_type, false });
  op.signature.params.push_back(ParameterDefinition{ "to_unit", unit_type, false });
  op.signature.outputs.push_back(ParameterDefinition{ "result", value_type, false });
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

TypeDefinition make_caliper_dimension() {
  TypeDefinition def;
  def.type_id = kTypeCaliperDimension;
  def.name = "Dimension";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "symbol", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "components", kTypeBytes, false, std::nullopt });
  add_compatible_operation(def, kTypeCaliperDimension);
  return def;
}

TypeDefinition make_caliper_unit() {
  TypeDefinition def;
  def.type_id = kTypeCaliperUnit;
  def.name = "Unit";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "name", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "symbol", kTypeString, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "dimension_id", kTypeObjectID, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "system", kTypeString, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "scale", kTypeF64, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "offset", kTypeF64, false, std::nullopt });
  def.fields.push_back(FieldDefinition{ "base_unit_id", kTypeObjectID, false, std::nullopt });
  add_compatible_operation(def, kTypeCaliperUnit);
  add_convert_operation(def, kTypeBytes, kTypeCaliperUnit);
  return def;
}

TypeDefinition make_caliper_angle() {
  TypeDefinition def;
  def.type_id = kTypeCaliperAngle;
  def.name = "Angle";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "unit_id", kTypeObjectID, false, std::nullopt });
  return def;
}

TypeDefinition make_caliper_duration() {
  TypeDefinition def;
  def.type_id = kTypeCaliperDuration;
  def.name = "Duration";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "unit_id", kTypeObjectID, false, std::nullopt });
  return def;
}

TypeDefinition make_caliper_span() {
  TypeDefinition def;
  def.type_id = kTypeCaliperSpan;
  def.name = "Span";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "unit_id", kTypeObjectID, false, std::nullopt });
  return def;
}

TypeDefinition make_caliper_range() {
  TypeDefinition def;
  def.type_id = kTypeCaliperRange;
  def.name = "Range";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "min_value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "max_value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "unit_id", kTypeObjectID, false, std::nullopt });
  return def;
}

TypeDefinition make_caliper_percentage() {
  TypeDefinition def;
  def.type_id = kTypeCaliperPercentage;
  def.name = "Percentage";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "unit_id", kTypeObjectID, false, std::nullopt });
  return def;
}

TypeDefinition make_caliper_ratio() {
  TypeDefinition def;
  def.type_id = kTypeCaliperRatio;
  def.name = "Ratio";
  def.namespace_name = "Caliper";
  def.version = 1;
  def.fields.push_back(FieldDefinition{ "value", kTypeBytes, true, std::nullopt });
  def.fields.push_back(FieldDefinition{ "unit_id", kTypeObjectID, false, std::nullopt });
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
  defs.reserve(44);
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
  defs.push_back(make_caliper_dimension());
  defs.push_back(make_caliper_unit());
  defs.push_back(make_caliper_angle());
  defs.push_back(make_caliper_duration());
  defs.push_back(make_caliper_span());
  defs.push_back(make_caliper_range());
  defs.push_back(make_caliper_percentage());
  defs.push_back(make_caliper_ratio());
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

namespace {

struct DimensionSeed {
  std::string name;
  std::string symbol;
  nlohmann::json components;
};

struct UnitSeed {
  std::string name;
  std::string symbol;
  std::string dimension;
  std::string system;
  std::optional<std::string> base_symbol;
  std::optional<double> scale;
  std::optional<double> offset;
};

referee::Result<DefinitionRecord> require_definition(SchemaRegistry& registry, referee::TypeID type_id) {
  auto defR = registry.get_definition_by_type(type_id);
  if (!defR) return referee::Result<DefinitionRecord>::err(defR.error->message);
  if (!defR.value->has_value()) return referee::Result<DefinitionRecord>::err("definition not found");
  return referee::Result<DefinitionRecord>::ok(defR.value->value());
}

std::map<std::string, referee::ObjectID> load_named_objects(
    referee::SqliteStore& store,
    referee::TypeID type_id,
    std::string_view key) {
  std::map<std::string, referee::ObjectID> out;
  auto listR = store.list_by_type(type_id);
  if (!listR) return out;
  for (const auto& rec : listR.value.value()) {
    try {
      auto j = nlohmann::json::from_cbor(rec.payload_cbor);
      if (!j.contains(key)) continue;
      auto name = j.at(key).get<std::string>();
      if (!name.empty()) out[name] = rec.ref.id;
    } catch (const std::exception&) {
      continue;
    }
  }
  return out;
}

referee::Bytes cbor_from_json(const nlohmann::json& j) {
  return nlohmann::json::to_cbor(j);
}

} // namespace

referee::Result<CatalogBootstrapResult> bootstrap_core_catalog(SchemaRegistry& registry,
                                                               referee::SqliteStore& store) {
  CatalogBootstrapResult out;

  auto dim_def = require_definition(registry, kTypeCaliperDimension);
  if (!dim_def) return referee::Result<CatalogBootstrapResult>::err(dim_def.error->message);
  auto unit_def = require_definition(registry, kTypeCaliperUnit);
  if (!unit_def) return referee::Result<CatalogBootstrapResult>::err(unit_def.error->message);

  std::map<std::string, referee::ObjectID> dimensions_by_name =
      load_named_objects(store, kTypeCaliperDimension, "name");
  std::map<std::string, referee::ObjectID> units_by_symbol =
      load_named_objects(store, kTypeCaliperUnit, "symbol");

  const std::vector<DimensionSeed> dimension_seeds = {
    { "Dimensionless", "1", nlohmann::json::object() },
    { "Length", "L", nlohmann::json{ { "Length", 1 } } },
    { "Mass", "M", nlohmann::json{ { "Mass", 1 } } },
    { "Time", "T", nlohmann::json{ { "Time", 1 } } },
    { "Angle", "Ang", nlohmann::json{ { "Angle", 1 } } },
    { "Temperature", "Temp", nlohmann::json{ { "Temperature", 1 } } },
    { "Area", "L2", nlohmann::json{ { "Length", 2 } } },
    { "Volume", "L3", nlohmann::json{ { "Length", 3 } } },
    { "Velocity", "L/T", nlohmann::json{ { "Length", 1 }, { "Time", -1 } } },
    { "Acceleration", "L/T2", nlohmann::json{ { "Length", 1 }, { "Time", -2 } } },
    { "Force", "M*L/T2", nlohmann::json{ { "Mass", 1 }, { "Length", 1 }, { "Time", -2 } } },
    { "Pressure", "M/L/T2", nlohmann::json{ { "Mass", 1 }, { "Length", -1 }, { "Time", -2 } } },
    { "Energy", "M*L2/T2", nlohmann::json{ { "Mass", 1 }, { "Length", 2 }, { "Time", -2 } } },
    { "Power", "M*L2/T3", nlohmann::json{ { "Mass", 1 }, { "Length", 2 }, { "Time", -3 } } },
  };

  for (const auto& seed : dimension_seeds) {
    auto it = dimensions_by_name.find(seed.name);
    if (it != dimensions_by_name.end()) {
      ++out.existing;
      continue;
    }
    nlohmann::json j;
    j["name"] = seed.name;
    j["symbol"] = seed.symbol;
    j["components"] = seed.components;
    auto createR = store.create_object(dim_def.value->definition.type_id,
                                       dim_def.value->ref.id,
                                       cbor_from_json(j));
    if (!createR) return referee::Result<CatalogBootstrapResult>::err(createR.error->message);
    dimensions_by_name[seed.name] = createR.value->ref.id;
    ++out.inserted;
  }

  auto require_dimension = [&](const std::string& name) -> referee::Result<referee::ObjectID> {
    auto it = dimensions_by_name.find(name);
    if (it == dimensions_by_name.end()) {
      return referee::Result<referee::ObjectID>::err("missing dimension: " + name);
    }
    return referee::Result<referee::ObjectID>::ok(it->second);
  };

  const std::vector<UnitSeed> unit_seeds = {
    { "one", "1", "Dimensionless", "si", std::nullopt, std::nullopt, std::nullopt },
    { "percent", "%", "Dimensionless", "si", std::string("1"), 0.01, 0.0 },

    { "meter", "m", "Length", "si", std::nullopt, std::nullopt, std::nullopt },
    { "millimeter", "mm", "Length", "si", std::string("m"), 0.001, 0.0 },
    { "centimeter", "cm", "Length", "si", std::string("m"), 0.01, 0.0 },
    { "kilometer", "km", "Length", "si", std::string("m"), 1000.0, 0.0 },
    { "inch", "in", "Length", "imperial", std::string("m"), 0.0254, 0.0 },
    { "foot", "ft", "Length", "imperial", std::string("m"), 0.3048, 0.0 },
    { "yard", "yd", "Length", "imperial", std::string("m"), 0.9144, 0.0 },
    { "mile", "mi", "Length", "imperial", std::string("m"), 1609.344, 0.0 },

    { "kilogram", "kg", "Mass", "si", std::nullopt, std::nullopt, std::nullopt },
    { "gram", "g", "Mass", "si", std::string("kg"), 0.001, 0.0 },
    { "tonne", "t", "Mass", "si", std::string("kg"), 1000.0, 0.0 },
    { "pound", "lb", "Mass", "imperial", std::string("kg"), 0.45359237, 0.0 },
    { "ounce", "oz", "Mass", "imperial", std::string("kg"), 0.028349523125, 0.0 },

    { "second", "s", "Time", "si", std::nullopt, std::nullopt, std::nullopt },
    { "minute", "min", "Time", "si", std::string("s"), 60.0, 0.0 },
    { "hour", "h", "Time", "si", std::string("s"), 3600.0, 0.0 },

    { "radian", "rad", "Angle", "si", std::nullopt, std::nullopt, std::nullopt },
    { "degree", "deg", "Angle", "si", std::string("rad"), 0.017453292519943295, 0.0 },

    { "kelvin", "K", "Temperature", "si", std::nullopt, std::nullopt, std::nullopt },
    { "celsius", "C", "Temperature", "si", std::string("K"), 1.0, 273.15 },
    { "fahrenheit", "F", "Temperature", "imperial", std::string("K"), 0.5555555555555556, 255.3722222222222 },

    { "square_meter", "m^2", "Area", "si", std::nullopt, std::nullopt, std::nullopt },
    { "square_foot", "ft^2", "Area", "imperial", std::string("m^2"), 0.09290304, 0.0 },

    { "cubic_meter", "m^3", "Volume", "si", std::nullopt, std::nullopt, std::nullopt },
    { "liter", "L", "Volume", "si", std::string("m^3"), 0.001, 0.0 },
    { "cubic_foot", "ft^3", "Volume", "imperial", std::string("m^3"), 0.028316846592, 0.0 },

    { "meter_per_second", "m/s", "Velocity", "si", std::nullopt, std::nullopt, std::nullopt },
    { "kilometer_per_hour", "km/h", "Velocity", "si", std::string("m/s"), 0.2777777777777778, 0.0 },
    { "mile_per_hour", "mph", "Velocity", "imperial", std::string("m/s"), 0.44704, 0.0 },

    { "meter_per_second_sq", "m/s^2", "Acceleration", "si", std::nullopt, std::nullopt, std::nullopt },

    { "newton", "N", "Force", "si", std::nullopt, std::nullopt, std::nullopt },
    { "pound_force", "lbf", "Force", "imperial", std::string("N"), 4.4482216152605, 0.0 },

    { "pascal", "Pa", "Pressure", "si", std::nullopt, std::nullopt, std::nullopt },
    { "bar", "bar", "Pressure", "si", std::string("Pa"), 100000.0, 0.0 },
    { "psi", "psi", "Pressure", "imperial", std::string("Pa"), 6894.757293168, 0.0 },

    { "joule", "J", "Energy", "si", std::nullopt, std::nullopt, std::nullopt },
    { "kilojoule", "kJ", "Energy", "si", std::string("J"), 1000.0, 0.0 },
    { "calorie", "cal", "Energy", "si", std::string("J"), 4.184, 0.0 },

    { "watt", "W", "Power", "si", std::nullopt, std::nullopt, std::nullopt },
    { "horsepower", "hp", "Power", "imperial", std::string("W"), 745.69987158227022, 0.0 },
  };

  for (const auto& seed : unit_seeds) {
    if (units_by_symbol.find(seed.symbol) != units_by_symbol.end()) {
      ++out.existing;
      continue;
    }
    auto dimR = require_dimension(seed.dimension);
    if (!dimR) return referee::Result<CatalogBootstrapResult>::err(dimR.error->message);

    nlohmann::json j;
    j["name"] = seed.name;
    j["symbol"] = seed.symbol;
    j["dimension_id"] = dimR.value->to_hex();
    if (!seed.system.empty()) j["system"] = seed.system;
    if (seed.scale.has_value()) j["scale"] = seed.scale.value();
    if (seed.offset.has_value()) j["offset"] = seed.offset.value();
    if (seed.base_symbol.has_value()) {
      auto base_it = units_by_symbol.find(seed.base_symbol.value());
      if (base_it != units_by_symbol.end()) {
        j["base_unit_id"] = base_it->second.to_hex();
      }
    }

    auto createR = store.create_object(unit_def.value->definition.type_id,
                                       unit_def.value->ref.id,
                                       cbor_from_json(j));
    if (!createR) return referee::Result<CatalogBootstrapResult>::err(createR.error->message);
    units_by_symbol[seed.symbol] = createR.value->ref.id;
    ++out.inserted;
  }

  return referee::Result<CatalogBootstrapResult>::ok(out);
}

} // namespace iris::refract
