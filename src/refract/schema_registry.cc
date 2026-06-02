#include "refract/schema_registry.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>
#include <unordered_set>

namespace iris::refract {

namespace {

static std::string field_constraint_kind_to_string(FieldConstraintKind kind) {
  switch (kind) {
    case FieldConstraintKind::Required:
      return "required";
    case FieldConstraintKind::NonEmpty:
      return "non_empty";
  }
  return "required";
}

static referee::Result<FieldConstraintKind> field_constraint_kind_from_string(std::string_view kind) {
  if (kind == "required") {
    return referee::Result<FieldConstraintKind>::ok(FieldConstraintKind::Required);
  }
  if (kind == "non_empty") {
    return referee::Result<FieldConstraintKind>::ok(FieldConstraintKind::NonEmpty);
  }
  return referee::Result<FieldConstraintKind>::err("unknown field constraint kind");
}

static std::string relationship_constraint_kind_to_string(RelationshipConstraintKind kind) {
  switch (kind) {
    case RelationshipConstraintKind::MinOccurs:
      return "min_occurs";
    case RelationshipConstraintKind::MaxOccurs:
      return "max_occurs";
  }
  return "min_occurs";
}

static referee::Result<RelationshipConstraintKind> relationship_constraint_kind_from_string(
    std::string_view kind) {
  if (kind == "min_occurs") {
    return referee::Result<RelationshipConstraintKind>::ok(RelationshipConstraintKind::MinOccurs);
  }
  if (kind == "max_occurs") {
    return referee::Result<RelationshipConstraintKind>::ok(RelationshipConstraintKind::MaxOccurs);
  }
  return referee::Result<RelationshipConstraintKind>::err("unknown relationship constraint kind");
}

static std::string operation_effect_kind_to_string(OperationEffectKind kind) {
  switch (kind) {
    case OperationEffectKind::Reads:
      return "reads";
    case OperationEffectKind::Writes:
      return "writes";
    case OperationEffectKind::Emits:
      return "emits";
    case OperationEffectKind::Schedules:
      return "schedules";
    case OperationEffectKind::UsesIo:
      return "uses_io";
    case OperationEffectKind::Custom:
      return "custom";
  }
  return "custom";
}

static referee::Result<OperationEffectKind> operation_effect_kind_from_string(
    std::string_view kind) {
  if (kind == "reads") return referee::Result<OperationEffectKind>::ok(OperationEffectKind::Reads);
  if (kind == "writes") return referee::Result<OperationEffectKind>::ok(OperationEffectKind::Writes);
  if (kind == "emits") return referee::Result<OperationEffectKind>::ok(OperationEffectKind::Emits);
  if (kind == "schedules") {
    return referee::Result<OperationEffectKind>::ok(OperationEffectKind::Schedules);
  }
  if (kind == "uses_io") return referee::Result<OperationEffectKind>::ok(OperationEffectKind::UsesIo);
  if (kind == "custom") return referee::Result<OperationEffectKind>::ok(OperationEffectKind::Custom);
  return referee::Result<OperationEffectKind>::err("unknown operation effect kind");
}

static nlohmann::json to_json(const DocumentationMetadata& documentation) {
  nlohmann::json j;
  if (documentation.summary.has_value()) j["summary"] = documentation.summary.value();
  if (!documentation.examples.empty()) {
    j["examples"] = nlohmann::json::array();
    for (const auto& example : documentation.examples) j["examples"].push_back(example);
  }
  return j;
}

static nlohmann::json to_json(const FieldConstraint& constraint) {
  nlohmann::json j;
  j["kind"] = field_constraint_kind_to_string(constraint.kind);
  return j;
}

static nlohmann::json to_json(const RelationshipConstraint& constraint) {
  nlohmann::json j;
  j["kind"] = relationship_constraint_kind_to_string(constraint.kind);
  j["value"] = constraint.value;
  return j;
}

static nlohmann::json to_json(const FieldDefinition& field) {
  nlohmann::json j;
  j["name"] = field.name;
  j["type_id"] = field.type.v;
  j["required"] = field.required;
  if (field.default_json.has_value()) j["default_json"] = field.default_json.value();
  if (field.documentation.has_value()) j["documentation"] = to_json(field.documentation.value());
  if (!field.constraints.empty()) {
    j["constraints"] = nlohmann::json::array();
    for (const auto& constraint : field.constraints) {
      j["constraints"].push_back(to_json(constraint));
    }
  }
  return j;
}

static nlohmann::json to_json(const OperationEffect& effect) {
  nlohmann::json j;
  j["kind"] = operation_effect_kind_to_string(effect.kind);
  if (!effect.target.empty()) j["target"] = effect.target;
  if (effect.description.has_value()) j["description"] = effect.description.value();
  return j;
}

static nlohmann::json to_json(const ParameterDefinition& param) {
  nlohmann::json j;
  j["name"] = param.name;
  j["type_id"] = param.type.v;
  j["optional"] = param.optional;
  return j;
}

static nlohmann::json to_json(const SignatureDefinition& sig) {
  nlohmann::json j;
  j["params"] = nlohmann::json::array();
  for (const auto& param : sig.params) j["params"].push_back(to_json(param));
  j["outputs"] = nlohmann::json::array();
  for (const auto& out : sig.outputs) j["outputs"].push_back(to_json(out));
  return j;
}

static std::string scope_to_string(OperationScope scope) {
  return scope == OperationScope::Class ? "class" : "object";
}

static OperationScope scope_from_string(std::string_view scope) {
  if (scope == "class") return OperationScope::Class;
  return OperationScope::Object;
}

static nlohmann::json to_json(const OperationDefinition& op) {
  nlohmann::json j;
  j["name"] = op.name;
  j["scope"] = scope_to_string(op.scope);
  j["signature"] = to_json(op.signature);
  if (!op.required_capabilities.empty()) {
    j["capabilities"] = nlohmann::json::array();
    for (const auto& cap : op.required_capabilities) j["capabilities"].push_back(cap);
  }
  if (!op.effects.empty()) {
    j["effects"] = nlohmann::json::array();
    for (const auto& effect : op.effects) j["effects"].push_back(to_json(effect));
  }
  if (op.documentation.has_value()) j["documentation"] = to_json(op.documentation.value());
  return j;
}

static nlohmann::json to_json(const RelationshipSpec& rel) {
  nlohmann::json j;
  j["role"] = rel.role;
  j["cardinality"] = rel.cardinality;
  j["target"] = rel.target;
  if (rel.documentation.has_value()) j["documentation"] = to_json(rel.documentation.value());
  if (!rel.constraints.empty()) {
    j["constraints"] = nlohmann::json::array();
    for (const auto& constraint : rel.constraints) {
      j["constraints"].push_back(to_json(constraint));
    }
  }
  return j;
}

static nlohmann::json to_json(const EnumValueDefinition& value) {
  nlohmann::json j;
  j["name"] = value.name;
  j["value_json"] = value.value_json;
  return j;
}

static nlohmann::json to_json(const PacketFieldDefinition& field) {
  nlohmann::json j;
  j["name"] = field.name;
  j["type_id"] = field.type.v;
  j["bit_width"] = field.bit_width;
  return j;
}

static nlohmann::json to_json(const CollectionElementDefinition& element) {
  nlohmann::json j;
  j["role"] = element.role;
  j["type_id"] = element.type.v;
  return j;
}

static std::string generic_arg_kind_to_string(GenericArgKind kind) {
  switch (kind) {
    case GenericArgKind::Type:
      return "type";
    case GenericArgKind::Value:
      return "value";
    case GenericArgKind::Variadic:
      return "variadic";
  }
  return "type";
}

static GenericArgKind generic_arg_kind_from_string(std::string_view kind) {
  if (kind == "value") return GenericArgKind::Value;
  if (kind == "variadic") return GenericArgKind::Variadic;
  return GenericArgKind::Type;
}

static nlohmann::json to_json(const GenericArg& arg) {
  nlohmann::json j;
  j["kind"] = generic_arg_kind_to_string(arg.kind);
  if (arg.kind == GenericArgKind::Type) {
    j["type_id"] = arg.type_id.v;
  } else if (arg.kind == GenericArgKind::Value) {
    j["value_type_id"] = arg.value_type.v;
    j["value_json"] = arg.value_json;
  } else {
    j["items"] = nlohmann::json::array();
    for (const auto& item : arg.items) j["items"].push_back(to_json(item));
  }
  return j;
}

static nlohmann::json to_json(const TypeDefinition& def) {
  nlohmann::json j;
  j["type_id"] = def.type_id.v;
  j["name"] = def.name;
  j["namespace"] = def.namespace_name;
  j["version"] = def.version;
  if (def.kind.has_value()) j["kind"] = def.kind.value();
  if (def.preferred_renderer.has_value()) j["preferred_renderer"] = def.preferred_renderer.value();
  if (def.documentation.has_value()) j["documentation"] = to_json(def.documentation.value());
  if (!def.base_types.empty()) {
    j["base_types"] = nlohmann::json::array();
    for (const auto& type : def.base_types) j["base_types"].push_back(type.v);
  }
  if (!def.interface_types.empty()) {
    j["interface_types"] = nlohmann::json::array();
    for (const auto& type : def.interface_types) j["interface_types"].push_back(type.v);
  }
  if (!def.type_params.empty()) {
    j["type_params"] = nlohmann::json::array();
    for (const auto& param : def.type_params) j["type_params"].push_back(param);
  }

  j["fields"] = nlohmann::json::array();
  for (const auto& field : def.fields) j["fields"].push_back(to_json(field));

  if (def.has_enum_value_type) j["enum_value_type"] = def.enum_value_type.v;
  if (!def.enum_values.empty()) {
    j["enum_values"] = nlohmann::json::array();
    for (const auto& value : def.enum_values) j["enum_values"].push_back(to_json(value));
  }
  if (def.packet_byte_order.has_value()) j["packet_byte_order"] = def.packet_byte_order.value();
  if (!def.packet_fields.empty()) {
    j["packet_fields"] = nlohmann::json::array();
    for (const auto& field : def.packet_fields) j["packet_fields"].push_back(to_json(field));
  }
  if (def.collection_kind.has_value()) j["collection_kind"] = def.collection_kind.value();
  if (!def.collection_elements.empty()) {
    j["collection_elements"] = nlohmann::json::array();
    for (const auto& element : def.collection_elements) {
      j["collection_elements"].push_back(to_json(element));
    }
  }

  j["operations"] = nlohmann::json::array();
  for (const auto& op : def.operations) j["operations"].push_back(to_json(op));

  j["relationships"] = nlohmann::json::array();
  for (const auto& rel : def.relationships) j["relationships"].push_back(to_json(rel));

  return j;
}

static bool has_field_constraint(const FieldDefinition& field, FieldConstraintKind kind) {
  for (const auto& constraint : field.constraints) {
    if (constraint.kind == kind) return true;
  }
  return false;
}

static void add_field_constraint(FieldDefinition& field, FieldConstraintKind kind) {
  if (has_field_constraint(field, kind)) return;
  field.constraints.push_back(FieldConstraint{ kind });
}

static std::optional<std::uint64_t> relationship_constraint_value(
    const RelationshipSpec& rel,
    RelationshipConstraintKind kind) {
  for (const auto& constraint : rel.constraints) {
    if (constraint.kind == kind) return constraint.value;
  }
  return std::nullopt;
}

static void add_relationship_constraint(RelationshipSpec& rel,
                                        RelationshipConstraintKind kind,
                                        std::uint64_t value) {
  if (relationship_constraint_value(rel, kind).has_value()) return;
  rel.constraints.push_back(RelationshipConstraint{ kind, value });
}

static void sort_field_constraints(std::vector<FieldConstraint>* constraints) {
  std::sort(constraints->begin(), constraints->end(),
            [](const FieldConstraint& lhs, const FieldConstraint& rhs) {
              return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
            });
}

static void sort_relationship_constraints(std::vector<RelationshipConstraint>* constraints) {
  std::sort(constraints->begin(), constraints->end(),
            [](const RelationshipConstraint& lhs, const RelationshipConstraint& rhs) {
              if (lhs.kind != rhs.kind) {
                return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
              }
              return lhs.value < rhs.value;
            });
}

static referee::Result<void> normalize_field_constraints(FieldDefinition* field) {
  if (field->required) add_field_constraint(*field, FieldConstraintKind::Required);

  bool seen_required = false;
  bool seen_non_empty = false;
  for (const auto& constraint : field->constraints) {
    if (constraint.kind == FieldConstraintKind::Required) {
      if (seen_required) {
        return referee::Result<void>::err("field has duplicate required constraint");
      }
      seen_required = true;
      field->required = true;
      continue;
    }
    if (constraint.kind == FieldConstraintKind::NonEmpty) {
      if (seen_non_empty) {
        return referee::Result<void>::err("field has duplicate non_empty constraint");
      }
      seen_non_empty = true;
    }
  }

  sort_field_constraints(&field->constraints);
  return referee::Result<void>::ok();
}

static referee::Result<void> normalize_relationship_constraints(RelationshipSpec* rel) {
  if (rel->cardinality == "one") {
    add_relationship_constraint(*rel, RelationshipConstraintKind::MinOccurs, 1);
    add_relationship_constraint(*rel, RelationshipConstraintKind::MaxOccurs, 1);
  } else if (rel->cardinality == "many") {
    add_relationship_constraint(*rel, RelationshipConstraintKind::MinOccurs, 0);
  } else if (!rel->cardinality.empty()) {
    return referee::Result<void>::err("relationship has unsupported cardinality");
  }

  bool seen_min = false;
  bool seen_max = false;
  std::optional<std::uint64_t> min_occurs;
  std::optional<std::uint64_t> max_occurs;
  for (const auto& constraint : rel->constraints) {
    if (constraint.kind == RelationshipConstraintKind::MinOccurs) {
      if (seen_min) {
        return referee::Result<void>::err("relationship has duplicate min_occurs constraint");
      }
      seen_min = true;
      min_occurs = constraint.value;
      continue;
    }
    if (constraint.kind == RelationshipConstraintKind::MaxOccurs) {
      if (seen_max) {
        return referee::Result<void>::err("relationship has duplicate max_occurs constraint");
      }
      seen_max = true;
      max_occurs = constraint.value;
    }
  }

  if (min_occurs.has_value() && max_occurs.has_value() && *min_occurs > *max_occurs) {
    return referee::Result<void>::err("relationship min_occurs exceeds max_occurs");
  }
  if (rel->cardinality == "one") {
    if (!min_occurs.has_value() || !max_occurs.has_value()
        || *min_occurs != 1 || *max_occurs != 1) {
      return referee::Result<void>::err("relationship cardinality 'one' requires exactly one");
    }
  }

  sort_relationship_constraints(&rel->constraints);
  return referee::Result<void>::ok();
}

static referee::Result<FieldConstraint> field_constraint_from_json(const nlohmann::json& j) {
  auto kind_text = j.is_string() ? j.get<std::string>() : j.value("kind", "");
  auto kindR = field_constraint_kind_from_string(kind_text);
  if (!kindR) {
    return referee::Result<FieldConstraint>::err("invalid field constraint kind: " + kind_text);
  }
  return referee::Result<FieldConstraint>::ok(FieldConstraint{ kindR.value.value() });
}

static referee::Result<RelationshipConstraint> relationship_constraint_from_json(
    const nlohmann::json& j) {
  if (!j.is_object()) {
    return referee::Result<RelationshipConstraint>::err(
        "relationship constraint must be an object");
  }
  auto kind_text = j.value("kind", "");
  auto kindR = relationship_constraint_kind_from_string(kind_text);
  if (!kindR) {
    return referee::Result<RelationshipConstraint>::err(
        "invalid relationship constraint kind: " + kind_text);
  }
  if (!j.contains("value")) {
    return referee::Result<RelationshipConstraint>::err(
        "relationship constraint missing value");
  }
  RelationshipConstraint constraint{};
  constraint.kind = kindR.value.value();
  constraint.value = j.at("value").get<std::uint64_t>();
  return referee::Result<RelationshipConstraint>::ok(std::move(constraint));
}

static DocumentationMetadata documentation_from_json(const nlohmann::json& j) {
  DocumentationMetadata documentation{};
  if (j.contains("summary")) documentation.summary = j.at("summary").get<std::string>();
  if (j.contains("examples")) {
    for (const auto& item : j.at("examples")) {
      documentation.examples.push_back(item.get<std::string>());
    }
  }
  return documentation;
}

static referee::Result<OperationEffect> operation_effect_from_json(const nlohmann::json& j) {
  if (!j.is_object()) {
    return referee::Result<OperationEffect>::err("operation effect must be an object");
  }
  auto kind_text = j.value("kind", "custom");
  auto kindR = operation_effect_kind_from_string(kind_text);
  if (!kindR) {
    return referee::Result<OperationEffect>::err(
        "invalid operation effect kind: " + kind_text);
  }

  OperationEffect effect{};
  effect.kind = kindR.value.value();
  effect.target = j.value("target", "");
  if (j.contains("description")) effect.description = j.at("description").get<std::string>();
  return referee::Result<OperationEffect>::ok(std::move(effect));
}

static referee::Result<FieldDefinition> field_from_json(const nlohmann::json& j) {
  FieldDefinition f{};
  f.name = j.value("name", "");
  f.type = referee::TypeID{j.value("type_id", 0ULL)};
  f.required = j.value("required", false);
  if (j.contains("default_json")) f.default_json = j.at("default_json").get<std::string>();
  if (j.contains("documentation")) f.documentation = documentation_from_json(j.at("documentation"));
  if (j.contains("constraints")) {
    for (const auto& item : j.at("constraints")) {
      auto constraintR = field_constraint_from_json(item);
      if (!constraintR) return referee::Result<FieldDefinition>::err(constraintR.error->message);
      f.constraints.push_back(constraintR.value.value());
    }
  }
  auto normalizeR = normalize_field_constraints(&f);
  if (!normalizeR) return referee::Result<FieldDefinition>::err(normalizeR.error->message);
  return referee::Result<FieldDefinition>::ok(std::move(f));
}

static ParameterDefinition param_from_json(const nlohmann::json& j) {
  ParameterDefinition p{};
  p.name = j.value("name", "");
  p.type = referee::TypeID{j.value("type_id", 0ULL)};
  p.optional = j.value("optional", false);
  return p;
}

static SignatureDefinition signature_from_json(const nlohmann::json& j) {
  SignatureDefinition sig{};
  if (j.contains("params")) {
    for (const auto& item : j.at("params")) sig.params.push_back(param_from_json(item));
  }
  if (j.contains("outputs")) {
    for (const auto& item : j.at("outputs")) sig.outputs.push_back(param_from_json(item));
  } else if (j.contains("return_type")) {
    ParameterDefinition out;
    out.name = "result";
    out.type = referee::TypeID{j.at("return_type").get<std::uint64_t>()};
    out.optional = false;
    sig.outputs.push_back(std::move(out));
  }
  return sig;
}

static referee::Result<OperationDefinition> operation_from_json(const nlohmann::json& j) {
  OperationDefinition op{};
  op.name = j.value("name", "");
  if (j.contains("scope")) op.scope = scope_from_string(j.at("scope").get<std::string>());
  if (j.contains("signature")) op.signature = signature_from_json(j.at("signature"));
  if (j.contains("capabilities")) {
    for (const auto& item : j.at("capabilities")) {
      op.required_capabilities.push_back(item.get<std::string>());
    }
  }
  if (j.contains("effects")) {
    for (const auto& item : j.at("effects")) {
      auto effectR = operation_effect_from_json(item);
      if (!effectR) return referee::Result<OperationDefinition>::err(effectR.error->message);
      op.effects.push_back(effectR.value.value());
    }
  }
  if (j.contains("documentation")) op.documentation = documentation_from_json(j.at("documentation"));
  return referee::Result<OperationDefinition>::ok(std::move(op));
}

static referee::Result<RelationshipSpec> relationship_from_json(const nlohmann::json& j) {
  RelationshipSpec rel{};
  rel.role = j.value("role", "");
  rel.cardinality = j.value("cardinality", "");
  rel.target = j.value("target", "");
  if (j.contains("documentation")) rel.documentation = documentation_from_json(j.at("documentation"));
  if (j.contains("constraints")) {
    for (const auto& item : j.at("constraints")) {
      auto constraintR = relationship_constraint_from_json(item);
      if (!constraintR) return referee::Result<RelationshipSpec>::err(constraintR.error->message);
      rel.constraints.push_back(constraintR.value.value());
    }
  }
  auto normalizeR = normalize_relationship_constraints(&rel);
  if (!normalizeR) return referee::Result<RelationshipSpec>::err(normalizeR.error->message);
  return referee::Result<RelationshipSpec>::ok(std::move(rel));
}

static EnumValueDefinition enum_value_from_json(const nlohmann::json& j) {
  EnumValueDefinition value{};
  value.name = j.value("name", "");
  value.value_json = j.value("value_json", "");
  return value;
}

static PacketFieldDefinition packet_field_from_json(const nlohmann::json& j) {
  PacketFieldDefinition field{};
  field.name = j.value("name", "");
  field.type = referee::TypeID{j.value("type_id", 0ULL)};
  field.bit_width = j.value("bit_width", 0U);
  return field;
}

static CollectionElementDefinition collection_element_from_json(const nlohmann::json& j) {
  CollectionElementDefinition element{};
  element.role = j.value("role", "");
  element.type = referee::TypeID{j.value("type_id", 0ULL)};
  return element;
}

static referee::Result<GenericArg> generic_arg_from_json(const nlohmann::json& j) {
  GenericArg arg{};
  arg.kind = generic_arg_kind_from_string(j.value("kind", "type"));
  if (arg.kind == GenericArgKind::Type) {
    arg.type_id = referee::TypeID{j.value("type_id", 0ULL)};
  } else if (arg.kind == GenericArgKind::Value) {
    arg.value_type = referee::TypeID{j.value("value_type_id", 0ULL)};
    arg.value_json = j.value("value_json", "");
  } else {
    if (j.contains("items")) {
      for (const auto& item : j.at("items")) {
        auto itemR = generic_arg_from_json(item);
        if (!itemR) return itemR;
        arg.items.push_back(itemR.value.value());
      }
    }
  }
  return referee::Result<GenericArg>::ok(std::move(arg));
}

static referee::Result<TypeDefinition> normalize_definition(TypeDefinition def) {
  for (auto& field : def.fields) {
    auto normalizeR = normalize_field_constraints(&field);
    if (!normalizeR) {
      return referee::Result<TypeDefinition>::err(
          "field '" + field.name + "': " + normalizeR.error->message);
    }
  }

  for (auto& rel : def.relationships) {
    auto normalizeR = normalize_relationship_constraints(&rel);
    if (!normalizeR) {
      return referee::Result<TypeDefinition>::err(
          "relationship '" + rel.role + "': " + normalizeR.error->message);
    }
  }

  return referee::Result<TypeDefinition>::ok(std::move(def));
}

static referee::Result<TypeDefinition> definition_from_json(const nlohmann::json& j) {
  TypeDefinition def{};
  def.type_id = referee::TypeID{j.value("type_id", 0ULL)};
  def.name = j.value("name", "");
  def.namespace_name = j.value("namespace", "");
  def.version = j.value("version", 1ULL);
  if (j.contains("kind")) def.kind = j.at("kind").get<std::string>();
  if (j.contains("preferred_renderer")) {
    def.preferred_renderer = j.at("preferred_renderer").get<std::string>();
  }
  if (j.contains("documentation")) def.documentation = documentation_from_json(j.at("documentation"));
  if (j.contains("base_types")) {
    for (const auto& item : j.at("base_types")) {
      def.base_types.push_back(referee::TypeID{item.get<std::uint64_t>()});
    }
  }
  if (j.contains("interface_types")) {
    for (const auto& item : j.at("interface_types")) {
      def.interface_types.push_back(referee::TypeID{item.get<std::uint64_t>()});
    }
  }
  if (j.contains("type_params")) {
    for (const auto& item : j.at("type_params")) def.type_params.push_back(item.get<std::string>());
  }

  if (j.contains("fields")) {
    for (const auto& item : j.at("fields")) {
      auto fieldR = field_from_json(item);
      if (!fieldR) return referee::Result<TypeDefinition>::err(fieldR.error->message);
      def.fields.push_back(fieldR.value.value());
    }
  }
  if (j.contains("enum_value_type")) {
    def.enum_value_type = referee::TypeID{j.at("enum_value_type").get<std::uint64_t>()};
    def.has_enum_value_type = true;
  }
  if (j.contains("enum_values")) {
    for (const auto& item : j.at("enum_values")) def.enum_values.push_back(enum_value_from_json(item));
  }
  if (j.contains("packet_byte_order")) {
    def.packet_byte_order = j.at("packet_byte_order").get<std::string>();
  }
  if (j.contains("packet_fields")) {
    for (const auto& item : j.at("packet_fields")) def.packet_fields.push_back(packet_field_from_json(item));
  }
  if (j.contains("collection_kind")) {
    def.collection_kind = j.at("collection_kind").get<std::string>();
  }
  if (j.contains("collection_elements")) {
    for (const auto& item : j.at("collection_elements")) {
      def.collection_elements.push_back(collection_element_from_json(item));
    }
  }
  if (j.contains("operations")) {
    for (const auto& item : j.at("operations")) {
      auto opR = operation_from_json(item);
      if (!opR) return referee::Result<TypeDefinition>::err(opR.error->message);
      def.operations.push_back(opR.value.value());
    }
  }
  if (j.contains("relationships")) {
    for (const auto& item : j.at("relationships")) {
      auto relR = relationship_from_json(item);
      if (!relR) return referee::Result<TypeDefinition>::err(relR.error->message);
      def.relationships.push_back(relR.value.value());
    }
  }

  return normalize_definition(std::move(def));
}

static referee::Result<std::string> canonicalize_value_json(const std::string& value_json) {
  try {
    auto j = nlohmann::json::parse(value_json);
    return referee::Result<std::string>::ok(j.dump());
  } catch (const std::exception& ex) {
    return referee::Result<std::string>::err(ex.what());
  }
}

static std::string hex_u64(std::uint64_t v) {
  std::array<char, 17> buf{};
  std::snprintf(buf.data(), buf.size(), "%016llx",
                static_cast<unsigned long long>(v));
  return std::string(buf.data());
}

static referee::Result<std::string> encode_generic_arg_key(const GenericArg& arg) {
  switch (arg.kind) {
    case GenericArgKind::Type:
      return referee::Result<std::string>::ok("type:0x" + hex_u64(arg.type_id.v));
    case GenericArgKind::Value: {
      auto canonR = canonicalize_value_json(arg.value_json);
      if (!canonR) return canonR;
      std::string key = "value:0x" + hex_u64(arg.value_type.v) + "=" + canonR.value.value();
      return referee::Result<std::string>::ok(std::move(key));
    }
    case GenericArgKind::Variadic: {
      std::string key = "variadic[";
      bool first = true;
      for (const auto& item : arg.items) {
        auto itemR = encode_generic_arg_key(item);
        if (!itemR) return itemR;
        if (!first) key += ",";
        key += itemR.value.value();
        first = false;
      }
      key += "]";
      return referee::Result<std::string>::ok(std::move(key));
    }
  }
  return referee::Result<std::string>::err("unknown generic arg kind");
}

static std::uint64_t fnv1a_64(std::string_view input) {
  constexpr std::uint64_t kOffset = 14695981039346656037ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t hash = kOffset;
  for (unsigned char c : input) {
    hash ^= c;
    hash *= kPrime;
  }
  return hash;
}

static referee::Result<GenericInstance> generic_instance_from_json(const nlohmann::json& j) {
  GenericInstance instance{};
  instance.base_type = referee::TypeID{j.value("base_type_id", 0ULL)};
  instance.instance_type = referee::TypeID{j.value("instance_type_id", 0ULL)};
  if (j.contains("display")) instance.display = j.at("display").get<std::string>();
  if (j.contains("args")) {
    nlohmann::json args_json;
    if (j.at("args").is_binary()) {
      const auto& bin = j.at("args").get_binary();
      args_json = nlohmann::json::from_cbor(bin);
    } else {
      args_json = j.at("args");
    }
    for (const auto& item : args_json) {
      auto argR = generic_arg_from_json(item);
      if (!argR) return referee::Result<GenericInstance>::err(argR.error->message);
      instance.args.push_back(argR.value.value());
    }
  }
  return referee::Result<GenericInstance>::ok(std::move(instance));
}

static nlohmann::json generic_instance_to_json(const GenericInstance& instance,
                                               std::string_view args_text) {
  nlohmann::json j;
  j["base_type_id"] = instance.base_type.v;
  j["instance_type_id"] = instance.instance_type.v;
  j["args_text"] = args_text;
  if (instance.display.has_value()) j["display"] = instance.display.value();
  nlohmann::json args_json = nlohmann::json::array();
  for (const auto& arg : instance.args) args_json.push_back(to_json(arg));
  auto args_cbor = nlohmann::json::to_cbor(args_json);
  j["args"] = nlohmann::json::binary(args_cbor);
  return j;
}

static referee::Result<TypeDefinition> decode_definition(const referee::Bytes& payload) {
  try {
    nlohmann::json j = nlohmann::json::from_cbor(payload);
    return definition_from_json(j);
  } catch (const std::exception& ex) {
    return referee::Result<TypeDefinition>::err(ex.what());
  }
}

static referee::Bytes encode_definition(const TypeDefinition& def) {
  return nlohmann::json::to_cbor(to_json(def));
}

static referee::Result<DefinitionRecord> record_from_object(const referee::ObjectRecord& rec) {
  auto defR = decode_definition(rec.payload_cbor);
  if (!defR) return referee::Result<DefinitionRecord>::err(defR.error->message);

  DefinitionRecord out{};
  out.ref = rec.ref;
  out.definition = defR.value.value();
  return referee::Result<DefinitionRecord>::ok(std::move(out));
}

static referee::Result<std::optional<std::string>> migration_hook_from_props(
    const referee::Bytes& props_cbor) {
  if (props_cbor.empty()) return referee::Result<std::optional<std::string>>::ok(std::nullopt);
  try {
    auto j = nlohmann::json::from_cbor(props_cbor);
    if (!j.contains("hook")) {
      return referee::Result<std::optional<std::string>>::err("migration_hook missing hook");
    }
    return referee::Result<std::optional<std::string>>::ok(j.at("hook").get<std::string>());
  } catch (const std::exception& ex) {
    return referee::Result<std::optional<std::string>>::err(ex.what());
  }
}

bool is_base_role(std::string_view role) {
  return role == "base" || role == "extends";
}

bool is_interface_role(std::string_view role) {
  return role == "implements" || role == "interface";
}

std::string type_display_name(const TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
}

referee::Result<std::vector<TypeSummary>> latest_type_summaries(SchemaRegistry& registry) {
  auto typesR = registry.list_types();
  if (!typesR) return referee::Result<std::vector<TypeSummary>>::err(typesR.error->message);

  std::map<std::uint64_t, TypeSummary> latest;
  for (const auto& summary : typesR.value.value()) {
    if (latest.find(summary.type_id.v) != latest.end()) continue;
    auto defR = registry.get_latest_definition_by_type(summary.type_id);
    if (!defR) return referee::Result<std::vector<TypeSummary>>::err(defR.error->message);
    if (!defR.value->has_value()) continue;

    TypeSummary latest_summary;
    latest_summary.type_id = summary.type_id;
    latest_summary.definition_id = defR.value->value().ref.id;
    latest_summary.name = defR.value->value().definition.name;
    latest_summary.namespace_name = defR.value->value().definition.namespace_name;
    latest_summary.preferred_renderer = defR.value->value().definition.preferred_renderer;
    latest_summary.documentation = defR.value->value().definition.documentation;
    latest[summary.type_id.v] = std::move(latest_summary);
  }

  std::vector<TypeSummary> out;
  out.reserve(latest.size());
  for (auto& [_, summary] : latest) {
    out.push_back(std::move(summary));
  }
  return referee::Result<std::vector<TypeSummary>>::ok(std::move(out));
}

std::optional<referee::TypeID> resolve_relationship_target(
    const std::vector<TypeSummary>& types,
    std::string_view target) {
  std::optional<referee::TypeID> exact_match;
  bool exact_ambiguous = false;
  std::optional<referee::TypeID> bare_match;
  bool bare_ambiguous = false;

  for (const auto& summary : types) {
    if (type_display_name(summary) == target) {
      if (exact_match.has_value() && exact_match->v != summary.type_id.v) {
        exact_ambiguous = true;
      } else {
        exact_match = summary.type_id;
      }
    }

    if (summary.name == target) {
      if (bare_match.has_value() && bare_match->v != summary.type_id.v) {
        bare_ambiguous = true;
      } else {
        bare_match = summary.type_id;
      }
    }
  }

  if (exact_match.has_value() && !exact_ambiguous) return exact_match;
  if (bare_match.has_value() && !bare_ambiguous) return bare_match;
  return std::nullopt;
}

referee::Result<std::vector<referee::TypeID>> legacy_relationship_types(
    SchemaRegistry& registry,
    const TypeDefinition& def,
    bool (*match_role)(std::string_view)) {
  auto typesR = latest_type_summaries(registry);
  if (!typesR) return referee::Result<std::vector<referee::TypeID>>::err(typesR.error->message);

  std::vector<referee::TypeID> out;
  std::unordered_set<std::uint64_t> seen;
  for (const auto& rel : def.relationships) {
    if (!match_role(rel.role)) continue;
    auto target = resolve_relationship_target(typesR.value.value(), rel.target);
    if (!target.has_value()) continue;
    if (seen.insert(target->v).second) out.push_back(*target);
  }
  return referee::Result<std::vector<referee::TypeID>>::ok(std::move(out));
}

referee::Result<std::vector<referee::TypeID>> merge_type_lists(
    const std::vector<referee::TypeID>& first,
    const std::vector<referee::TypeID>& second) {
  std::vector<referee::TypeID> out;
  std::unordered_set<std::uint64_t> seen;
  out.reserve(first.size() + second.size());
  for (const auto& type : first) {
    if (seen.insert(type.v).second) out.push_back(type);
  }
  for (const auto& type : second) {
    if (seen.insert(type.v).second) out.push_back(type);
  }
  return referee::Result<std::vector<referee::TypeID>>::ok(std::move(out));
}

} // namespace

referee::Result<std::string> encode_generic_instance_key(const GenericInstance& instance) {
  if (instance.base_type.v == 0) {
    return referee::Result<std::string>::err("generic instance base type is zero");
  }
  std::string key = "base=0x" + hex_u64(instance.base_type.v) + ";args=[";
  bool first = true;
  for (const auto& arg : instance.args) {
    auto argR = encode_generic_arg_key(arg);
    if (!argR) return argR;
    if (!first) key += ",";
    key += argR.value.value();
    first = false;
  }
  key += "]";
  return referee::Result<std::string>::ok(std::move(key));
}

referee::Result<referee::TypeID> derive_generic_type_id(const GenericInstance& instance) {
  auto keyR = encode_generic_instance_key(instance);
  if (!keyR) return referee::Result<referee::TypeID>::err(keyR.error->message);
  return referee::Result<referee::TypeID>::ok(referee::TypeID{fnv1a_64(keyR.value.value())});
}

SchemaRegistry::SchemaRegistry(referee::SqliteStore& store) : store_(store) {}

referee::Result<DefinitionRecord> SchemaRegistry::register_definition(const TypeDefinition& def) {
  if (def.name.empty()) return referee::Result<DefinitionRecord>::err("definition name is empty");
  if (def.type_id.v == 0) return referee::Result<DefinitionRecord>::err("type_id is zero");

  auto normalizedR = normalize_definition(def);
  if (!normalizedR) return referee::Result<DefinitionRecord>::err(normalizedR.error->message);

  const auto& normalized = normalizedR.value.value();
  auto payload = encode_definition(normalized);
  auto definition_id = referee::ObjectID::random();
  auto createR = store_.create_object_with_id(definition_id, kTypeDefinitionType, definition_id,
                                              payload);
  if (!createR) return referee::Result<DefinitionRecord>::err(createR.error->message);

  if (normalized.supersedes_definition_id.has_value()) {
    auto priorR = store_.get_latest(normalized.supersedes_definition_id.value());
    if (!priorR) return referee::Result<DefinitionRecord>::err(priorR.error->message);
    if (!priorR.value->has_value()) {
      return referee::Result<DefinitionRecord>::err("supersedes definition not found");
    }
    auto edgeR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                 "supersedes", "definition", {});
    if (!edgeR) return referee::Result<DefinitionRecord>::err(edgeR.error->message);
    if (normalized.migration_hook.has_value()) {
      auto hookProps = referee::cbor_from_json_kv("hook", normalized.migration_hook.value());
      auto hookR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                   "migration_hook", "definition", hookProps);
      if (!hookR) return referee::Result<DefinitionRecord>::err(hookR.error->message);
    }
  } else if (normalized.migration_hook.has_value()) {
    return referee::Result<DefinitionRecord>::err("migration_hook requires supersedes_definition_id");
  }

  return record_from_object(createR.value.value());
}

referee::Result<DefinitionRecord> SchemaRegistry::register_definition_with_id(
    const TypeDefinition& def, referee::ObjectID definition_id) {
  if (def.name.empty()) return referee::Result<DefinitionRecord>::err("definition name is empty");
  if (def.type_id.v == 0) return referee::Result<DefinitionRecord>::err("type_id is zero");

  auto normalizedR = normalize_definition(def);
  if (!normalizedR) return referee::Result<DefinitionRecord>::err(normalizedR.error->message);

  const auto& normalized = normalizedR.value.value();
  auto payload = encode_definition(normalized);
  auto createR = store_.create_object_with_id(definition_id, kTypeDefinitionType, definition_id,
                                              payload);
  if (!createR) return referee::Result<DefinitionRecord>::err(createR.error->message);

  if (normalized.supersedes_definition_id.has_value()) {
    auto priorR = store_.get_latest(normalized.supersedes_definition_id.value());
    if (!priorR) return referee::Result<DefinitionRecord>::err(priorR.error->message);
    if (!priorR.value->has_value()) {
      return referee::Result<DefinitionRecord>::err("supersedes definition not found");
    }
    auto edgeR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                 "supersedes", "definition", {});
    if (!edgeR) return referee::Result<DefinitionRecord>::err(edgeR.error->message);
    if (normalized.migration_hook.has_value()) {
      auto hookProps = referee::cbor_from_json_kv("hook", normalized.migration_hook.value());
      auto hookR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                   "migration_hook", "definition", hookProps);
      if (!hookR) return referee::Result<DefinitionRecord>::err(hookR.error->message);
    }
  } else if (normalized.migration_hook.has_value()) {
    return referee::Result<DefinitionRecord>::err("migration_hook requires supersedes_definition_id");
  }

  return record_from_object(createR.value.value());
}

referee::Result<std::optional<DefinitionRecord>> SchemaRegistry::get_definition_by_id(referee::ObjectID id) {
  auto recR = store_.get_latest(id);
  if (!recR) return referee::Result<std::optional<DefinitionRecord>>::err(recR.error->message);
  if (!recR.value->has_value()) {
    return referee::Result<std::optional<DefinitionRecord>>::ok(std::optional<DefinitionRecord>{});
  }

  const auto& rec = recR.value->value();
  if (rec.type.v != kTypeDefinitionType.v) {
    return referee::Result<std::optional<DefinitionRecord>>::err("object is not a type definition");
  }

  auto defR = record_from_object(rec);
  if (!defR) return referee::Result<std::optional<DefinitionRecord>>::err(defR.error->message);

  return referee::Result<std::optional<DefinitionRecord>>::ok(defR.value.value());
}

referee::Result<std::optional<DefinitionRecord>> SchemaRegistry::get_definition_by_type(referee::TypeID type) {
  auto listR = store_.list_by_type(kTypeDefinitionType);
  if (!listR) return referee::Result<std::optional<DefinitionRecord>>::err(listR.error->message);

  for (const auto& rec : listR.value.value()) {
    auto defR = record_from_object(rec);
    if (!defR) return referee::Result<std::optional<DefinitionRecord>>::err(defR.error->message);
    if (defR.value->definition.type_id == type) {
      return referee::Result<std::optional<DefinitionRecord>>::ok(defR.value.value());
    }
  }

  return referee::Result<std::optional<DefinitionRecord>>::ok(std::optional<DefinitionRecord>{});
}

referee::Result<std::optional<DefinitionRecord>> SchemaRegistry::get_latest_definition_by_type(
    referee::TypeID type) {
  auto listR = store_.list_by_type(kTypeDefinitionType);
  if (!listR) return referee::Result<std::optional<DefinitionRecord>>::err(listR.error->message);

  std::optional<DefinitionRecord> latest;
  for (const auto& rec : listR.value.value()) {
    auto defR = record_from_object(rec);
    if (!defR) return referee::Result<std::optional<DefinitionRecord>>::err(defR.error->message);
    if (defR.value->definition.type_id != type) continue;

    if (!latest.has_value()
        || defR.value->definition.version > latest->definition.version) {
      latest = defR.value.value();
    }
  }

  return referee::Result<std::optional<DefinitionRecord>>::ok(std::move(latest));
}

referee::Result<std::vector<TypeSummary>> SchemaRegistry::list_types() {
  auto listR = store_.list_by_type(kTypeDefinitionType);
  if (!listR) return referee::Result<std::vector<TypeSummary>>::err(listR.error->message);

  std::vector<TypeSummary> out;
  out.reserve(listR.value->size());

  for (const auto& rec : listR.value.value()) {
    auto defR = record_from_object(rec);
    if (!defR) return referee::Result<std::vector<TypeSummary>>::err(defR.error->message);

    TypeSummary summary;
    summary.type_id = defR.value->definition.type_id;
    summary.definition_id = defR.value->ref.id;
    summary.name = defR.value->definition.name;
    summary.namespace_name = defR.value->definition.namespace_name;
    summary.preferred_renderer = defR.value->definition.preferred_renderer;
    summary.documentation = defR.value->definition.documentation;

    out.push_back(std::move(summary));
  }

  return referee::Result<std::vector<TypeSummary>>::ok(std::move(out));
}

referee::Result<std::vector<SupersedesLink>> SchemaRegistry::list_supersedes_chain(
    referee::ObjectID definition_id) {
  auto currentR = store_.get_latest(definition_id);
  if (!currentR) return referee::Result<std::vector<SupersedesLink>>::err(currentR.error->message);
  if (!currentR.value->has_value()) {
    return referee::Result<std::vector<SupersedesLink>>::err("definition not found");
  }

  auto current = currentR.value->value();
  if (current.type.v != kTypeDefinitionType.v) {
    return referee::Result<std::vector<SupersedesLink>>::err("object is not a type definition");
  }

  std::vector<SupersedesLink> chain;

  while (true) {
    auto edgesR = store_.edges_from(current.ref, "supersedes", "definition");
    if (!edgesR) return referee::Result<std::vector<SupersedesLink>>::err(edgesR.error->message);
    if (edgesR.value->empty()) break;
    if (edgesR.value->size() > 1) {
      return referee::Result<std::vector<SupersedesLink>>::err("multiple supersedes edges found");
    }

    const auto& edge = edgesR.value->front();
    auto priorRecR = store_.get_object(edge.to);
    if (!priorRecR) return referee::Result<std::vector<SupersedesLink>>::err(priorRecR.error->message);
    if (!priorRecR.value->has_value()) {
      return referee::Result<std::vector<SupersedesLink>>::err("supersedes target not found");
    }
    if (priorRecR.value->value().type.v != kTypeDefinitionType.v) {
      return referee::Result<std::vector<SupersedesLink>>::err("supersedes target is not a type definition");
    }

    auto priorDefR = record_from_object(priorRecR.value->value());
    if (!priorDefR) return referee::Result<std::vector<SupersedesLink>>::err(priorDefR.error->message);

    SupersedesLink link;
    link.prior = priorDefR.value.value();

    auto hookEdgesR = store_.edges_from(current.ref, "migration_hook", "definition");
    if (!hookEdgesR) return referee::Result<std::vector<SupersedesLink>>::err(hookEdgesR.error->message);
    for (const auto& hookEdge : hookEdgesR.value.value()) {
      if (hookEdge.to != edge.to) continue;
      if (link.migration_hook.has_value()) {
        return referee::Result<std::vector<SupersedesLink>>::err("multiple migration hooks found");
      }
      auto hookR = migration_hook_from_props(hookEdge.props_cbor);
      if (!hookR) return referee::Result<std::vector<SupersedesLink>>::err(hookR.error->message);
      link.migration_hook = hookR.value.value();
    }

    chain.push_back(std::move(link));
    current = priorRecR.value->value();
  }

  return referee::Result<std::vector<SupersedesLink>>::ok(std::move(chain));
}

referee::Result<std::vector<referee::TypeID>> SchemaRegistry::list_base_types(referee::TypeID type) {
  auto defR = get_latest_definition_by_type(type);
  if (!defR) return referee::Result<std::vector<referee::TypeID>>::err(defR.error->message);
  if (!defR.value->has_value()) {
    return referee::Result<std::vector<referee::TypeID>>::err("definition not found");
  }

  const auto& def = defR.value->value().definition;
  if (!def.base_types.empty()) {
    return referee::Result<std::vector<referee::TypeID>>::ok(def.base_types);
  }

  return legacy_relationship_types(*this, def, is_base_role);
}

referee::Result<std::vector<referee::TypeID>> SchemaRegistry::list_interface_types(
    referee::TypeID type) {
  auto defR = get_latest_definition_by_type(type);
  if (!defR) return referee::Result<std::vector<referee::TypeID>>::err(defR.error->message);
  if (!defR.value->has_value()) {
    return referee::Result<std::vector<referee::TypeID>>::err("definition not found");
  }

  const auto& def = defR.value->value().definition;
  if (!def.interface_types.empty()) {
    return referee::Result<std::vector<referee::TypeID>>::ok(def.interface_types);
  }

  return legacy_relationship_types(*this, def, is_interface_role);
}

referee::Result<std::vector<referee::TypeID>> SchemaRegistry::list_supertypes(referee::TypeID type) {
  auto baseR = list_base_types(type);
  if (!baseR) return referee::Result<std::vector<referee::TypeID>>::err(baseR.error->message);

  auto interfaceR = list_interface_types(type);
  if (!interfaceR) {
    return referee::Result<std::vector<referee::TypeID>>::err(interfaceR.error->message);
  }

  return merge_type_lists(baseR.value.value(), interfaceR.value.value());
}

GenericRegistry::GenericRegistry(SchemaRegistry& schema, referee::SqliteStore& store)
    : schema_(schema), store_(store) {}

referee::Result<GenericInstanceRecord> GenericRegistry::register_instance(
    const GenericInstance& instance) {
  auto defR = schema_.get_definition_by_type(kTypeGenericInstanceType);
  if (!defR) return referee::Result<GenericInstanceRecord>::err(defR.error->message);
  if (!defR.value->has_value()) {
    return referee::Result<GenericInstanceRecord>::err("generic instance definition missing");
  }

  auto keyR = encode_generic_instance_key(instance);
  if (!keyR) return referee::Result<GenericInstanceRecord>::err(keyR.error->message);
  auto typeR = derive_generic_type_id(instance);
  if (!typeR) return referee::Result<GenericInstanceRecord>::err(typeR.error->message);

  GenericInstance stored = instance;
  stored.instance_type = typeR.value.value();
  auto payload = nlohmann::json::to_cbor(generic_instance_to_json(stored, keyR.value.value()));

  auto createR = store_.create_object(kTypeGenericInstanceType, defR.value->value().ref.id, payload);
  if (!createR) return referee::Result<GenericInstanceRecord>::err(createR.error->message);

  GenericInstanceRecord record{};
  record.ref = createR.value->ref;
  record.instance = std::move(stored);
  return referee::Result<GenericInstanceRecord>::ok(std::move(record));
}

referee::Result<std::optional<GenericInstanceRecord>> GenericRegistry::get_instance_by_type(
    referee::TypeID type_id) {
  auto listR = store_.list_by_type(kTypeGenericInstanceType);
  if (!listR) return referee::Result<std::optional<GenericInstanceRecord>>::err(listR.error->message);
  for (const auto& rec : listR.value.value()) {
    try {
      auto j = nlohmann::json::from_cbor(rec.payload_cbor);
      auto instR = generic_instance_from_json(j);
      if (!instR) return referee::Result<std::optional<GenericInstanceRecord>>::err(instR.error->message);
      if (instR.value->instance_type == type_id) {
        GenericInstanceRecord record{};
        record.ref = rec.ref;
        record.instance = instR.value.value();
        return referee::Result<std::optional<GenericInstanceRecord>>::ok(record);
      }
    } catch (const std::exception& ex) {
      return referee::Result<std::optional<GenericInstanceRecord>>::err(ex.what());
    }
  }
  return referee::Result<std::optional<GenericInstanceRecord>>::ok(
      std::optional<GenericInstanceRecord>{});
}

ScopedTypeRegistry::ScopedTypeRegistry(Scope scope,
                                       GenericRegistry& registry,
                                       ScopedTypeRegistry* parent)
    : scope_(scope), registry_(registry), parent_(parent) {}

void ScopedTypeRegistry::set_logger(std::function<void(const std::string&)> logger) {
  logger_ = std::move(logger);
}

referee::Result<GenericInstanceRecord> ScopedTypeRegistry::resolve_or_register(
    const GenericInstance& instance,
    PromotionPolicy policy) {
  auto typeR = derive_generic_type_id(instance);
  if (!typeR) return referee::Result<GenericInstanceRecord>::err(typeR.error->message);

  auto foundR = find(typeR.value.value());
  if (!foundR) return referee::Result<GenericInstanceRecord>::err(foundR.error->message);
  if (foundR.value->has_value()) {
    return referee::Result<GenericInstanceRecord>::ok(foundR.value->value());
  }

  auto regR = registry_.register_instance(instance);
  if (!regR) return regR;

  cache_instance(regR.value.value());

  if (policy == PromotionPolicy::Parent && parent_) {
    promote_to(parent_, regR.value.value());
  } else if (policy == PromotionPolicy::Root) {
    auto root_scope = root();
    if (root_scope && root_scope != this) {
      promote_to(root_scope, regR.value.value());
    }
  }

  return regR;
}

referee::Result<std::optional<GenericInstanceRecord>> ScopedTypeRegistry::find(
    referee::TypeID type_id) {
  auto local = find_local(type_id);
  if (local.has_value()) {
    return referee::Result<std::optional<GenericInstanceRecord>>::ok(local);
  }
  if (parent_) {
    auto parentR = parent_->find(type_id);
    if (!parentR) return parentR;
    if (parentR.value->has_value()) return parentR;
  }
  return registry_.get_instance_by_type(type_id);
}

std::optional<GenericInstanceRecord> ScopedTypeRegistry::find_local(referee::TypeID type_id) const {
  auto it = cache_.find(type_id);
  if (it == cache_.end()) return std::nullopt;
  return it->second;
}

void ScopedTypeRegistry::cache_instance(const GenericInstanceRecord& record) {
  cache_[record.instance.instance_type] = record;
}

ScopedTypeRegistry* ScopedTypeRegistry::root() {
  ScopedTypeRegistry* current = this;
  while (current->parent_) {
    current = current->parent_;
  }
  return current;
}

void ScopedTypeRegistry::promote_to(ScopedTypeRegistry* target, const GenericInstanceRecord& record) {
  if (!target) return;
  if (target->find_local(record.instance.instance_type).has_value()) return;
  target->cache_instance(record);
  log_promotion(scope_, target->scope_, record.instance.instance_type);
}

void ScopedTypeRegistry::log_promotion(Scope from, Scope to, referee::TypeID type_id) {
  if (!logger_) return;
  auto scope_label = [](Scope scope) {
    switch (scope) {
      case Scope::Operation:
        return "operation";
      case Scope::Application:
        return "application";
      case Scope::Database:
        return "database";
      case Scope::Sandbox:
        return "sandbox";
      case Scope::Global:
        return "global";
    }
    return "unknown";
  };
  std::string msg = "promoted generic instance 0x" + hex_u64(type_id.v)
      + " from " + scope_label(from) + " to " + scope_label(to);
  logger_(msg);
}

} // namespace iris::refract
