#include "refract/schema_registry.h"

#include <nlohmann/json.hpp>
#include <string_view>

namespace iris::refract {

namespace {

static nlohmann::json to_json(const FieldDefinition& field) {
  nlohmann::json j;
  j["name"] = field.name;
  j["type_id"] = field.type.v;
  j["required"] = field.required;
  if (field.default_json.has_value()) j["default_json"] = field.default_json.value();
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
  return j;
}

static nlohmann::json to_json(const RelationshipSpec& rel) {
  nlohmann::json j;
  j["role"] = rel.role;
  j["cardinality"] = rel.cardinality;
  j["target"] = rel.target;
  return j;
}

static nlohmann::json to_json(const TypeDefinition& def) {
  nlohmann::json j;
  j["type_id"] = def.type_id.v;
  j["name"] = def.name;
  j["namespace"] = def.namespace_name;
  j["version"] = def.version;
  if (def.preferred_renderer.has_value()) j["preferred_renderer"] = def.preferred_renderer.value();

  j["fields"] = nlohmann::json::array();
  for (const auto& field : def.fields) j["fields"].push_back(to_json(field));

  j["operations"] = nlohmann::json::array();
  for (const auto& op : def.operations) j["operations"].push_back(to_json(op));

  j["relationships"] = nlohmann::json::array();
  for (const auto& rel : def.relationships) j["relationships"].push_back(to_json(rel));

  return j;
}

static FieldDefinition field_from_json(const nlohmann::json& j) {
  FieldDefinition f;
  f.name = j.value("name", "");
  f.type = referee::TypeID{j.value("type_id", 0ULL)};
  f.required = j.value("required", false);
  if (j.contains("default_json")) f.default_json = j.at("default_json").get<std::string>();
  return f;
}

static ParameterDefinition param_from_json(const nlohmann::json& j) {
  ParameterDefinition p;
  p.name = j.value("name", "");
  p.type = referee::TypeID{j.value("type_id", 0ULL)};
  p.optional = j.value("optional", false);
  return p;
}

static SignatureDefinition signature_from_json(const nlohmann::json& j) {
  SignatureDefinition sig;
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

static OperationDefinition operation_from_json(const nlohmann::json& j) {
  OperationDefinition op;
  op.name = j.value("name", "");
  if (j.contains("scope")) op.scope = scope_from_string(j.at("scope").get<std::string>());
  if (j.contains("signature")) op.signature = signature_from_json(j.at("signature"));
  return op;
}

static RelationshipSpec relationship_from_json(const nlohmann::json& j) {
  RelationshipSpec rel;
  rel.role = j.value("role", "");
  rel.cardinality = j.value("cardinality", "");
  rel.target = j.value("target", "");
  return rel;
}

static TypeDefinition definition_from_json(const nlohmann::json& j) {
  TypeDefinition def;
  def.type_id = referee::TypeID{j.value("type_id", 0ULL)};
  def.name = j.value("name", "");
  def.namespace_name = j.value("namespace", "");
  def.version = j.value("version", 1ULL);
  if (j.contains("preferred_renderer")) {
    def.preferred_renderer = j.at("preferred_renderer").get<std::string>();
  }

  if (j.contains("fields")) {
    for (const auto& item : j.at("fields")) def.fields.push_back(field_from_json(item));
  }
  if (j.contains("operations")) {
    for (const auto& item : j.at("operations")) def.operations.push_back(operation_from_json(item));
  }
  if (j.contains("relationships")) {
    for (const auto& item : j.at("relationships")) def.relationships.push_back(relationship_from_json(item));
  }

  return def;
}

static referee::Result<TypeDefinition> decode_definition(const referee::Bytes& payload) {
  try {
    nlohmann::json j = nlohmann::json::from_cbor(payload);
    return referee::Result<TypeDefinition>::ok(definition_from_json(j));
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

  DefinitionRecord out;
  out.ref = rec.ref;
  out.definition = std::move(defR.value.value());
  return referee::Result<DefinitionRecord>::ok(std::move(out));
}

} // namespace

SchemaRegistry::SchemaRegistry(referee::SqliteStore& store) : store_(store) {}

referee::Result<DefinitionRecord> SchemaRegistry::register_definition(const TypeDefinition& def) {
  if (def.name.empty()) return referee::Result<DefinitionRecord>::err("definition name is empty");
  if (def.type_id.v == 0) return referee::Result<DefinitionRecord>::err("type_id is zero");

  auto payload = encode_definition(def);
  auto definition_id = referee::ObjectID::random();
  auto createR = store_.create_object_with_id(definition_id, kTypeDefinitionType, definition_id, payload);
  if (!createR) return referee::Result<DefinitionRecord>::err(createR.error->message);

  return record_from_object(createR.value.value());
}

referee::Result<DefinitionRecord> SchemaRegistry::register_definition_with_id(
    const TypeDefinition& def, referee::ObjectID definition_id) {
  if (def.name.empty()) return referee::Result<DefinitionRecord>::err("definition name is empty");
  if (def.type_id.v == 0) return referee::Result<DefinitionRecord>::err("type_id is zero");

  auto payload = encode_definition(def);
  auto createR = store_.create_object_with_id(definition_id, kTypeDefinitionType, definition_id, payload);
  if (!createR) return referee::Result<DefinitionRecord>::err(createR.error->message);

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

    out.push_back(std::move(summary));
  }

  return referee::Result<std::vector<TypeSummary>>::ok(std::move(out));
}

} // namespace iris::refract
