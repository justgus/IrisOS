#include "refract/schema_registry.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdio>
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

static FieldDefinition field_from_json(const nlohmann::json& j) {
  FieldDefinition f{};
  f.name = j.value("name", "");
  f.type = referee::TypeID{j.value("type_id", 0ULL)};
  f.required = j.value("required", false);
  if (j.contains("default_json")) f.default_json = j.at("default_json").get<std::string>();
  return f;
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

static OperationDefinition operation_from_json(const nlohmann::json& j) {
  OperationDefinition op{};
  op.name = j.value("name", "");
  if (j.contains("scope")) op.scope = scope_from_string(j.at("scope").get<std::string>());
  if (j.contains("signature")) op.signature = signature_from_json(j.at("signature"));
  return op;
}

static RelationshipSpec relationship_from_json(const nlohmann::json& j) {
  RelationshipSpec rel{};
  rel.role = j.value("role", "");
  rel.cardinality = j.value("cardinality", "");
  rel.target = j.value("target", "");
  return rel;
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

static TypeDefinition definition_from_json(const nlohmann::json& j) {
  TypeDefinition def{};
  def.type_id = referee::TypeID{j.value("type_id", 0ULL)};
  def.name = j.value("name", "");
  def.namespace_name = j.value("namespace", "");
  def.version = j.value("version", 1ULL);
  if (j.contains("kind")) def.kind = j.at("kind").get<std::string>();
  if (j.contains("preferred_renderer")) {
    def.preferred_renderer = j.at("preferred_renderer").get<std::string>();
  }
  if (j.contains("type_params")) {
    for (const auto& item : j.at("type_params")) def.type_params.push_back(item.get<std::string>());
  }

  if (j.contains("fields")) {
    for (const auto& item : j.at("fields")) def.fields.push_back(field_from_json(item));
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
    for (const auto& item : j.at("operations")) def.operations.push_back(operation_from_json(item));
  }
  if (j.contains("relationships")) {
    for (const auto& item : j.at("relationships")) def.relationships.push_back(relationship_from_json(item));
  }

  return def;
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

  auto payload = encode_definition(def);
  auto definition_id = referee::ObjectID::random();
  auto createR = store_.create_object_with_id(definition_id, kTypeDefinitionType, definition_id,
                                              payload);
  if (!createR) return referee::Result<DefinitionRecord>::err(createR.error->message);

  if (def.supersedes_definition_id.has_value()) {
    auto priorR = store_.get_latest(def.supersedes_definition_id.value());
    if (!priorR) return referee::Result<DefinitionRecord>::err(priorR.error->message);
    if (!priorR.value->has_value()) {
      return referee::Result<DefinitionRecord>::err("supersedes definition not found");
    }
    auto edgeR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                 "supersedes", "definition", {});
    if (!edgeR) return referee::Result<DefinitionRecord>::err(edgeR.error->message);
    if (def.migration_hook.has_value()) {
      auto hookProps = referee::cbor_from_json_kv("hook", def.migration_hook.value());
      auto hookR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                   "migration_hook", "definition", hookProps);
      if (!hookR) return referee::Result<DefinitionRecord>::err(hookR.error->message);
    }
  } else if (def.migration_hook.has_value()) {
    return referee::Result<DefinitionRecord>::err("migration_hook requires supersedes_definition_id");
  }

  return record_from_object(createR.value.value());
}

referee::Result<DefinitionRecord> SchemaRegistry::register_definition_with_id(
    const TypeDefinition& def, referee::ObjectID definition_id) {
  if (def.name.empty()) return referee::Result<DefinitionRecord>::err("definition name is empty");
  if (def.type_id.v == 0) return referee::Result<DefinitionRecord>::err("type_id is zero");

  auto payload = encode_definition(def);
  auto createR = store_.create_object_with_id(definition_id, kTypeDefinitionType, definition_id,
                                              payload);
  if (!createR) return referee::Result<DefinitionRecord>::err(createR.error->message);

  if (def.supersedes_definition_id.has_value()) {
    auto priorR = store_.get_latest(def.supersedes_definition_id.value());
    if (!priorR) return referee::Result<DefinitionRecord>::err(priorR.error->message);
    if (!priorR.value->has_value()) {
      return referee::Result<DefinitionRecord>::err("supersedes definition not found");
    }
    auto edgeR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                 "supersedes", "definition", {});
    if (!edgeR) return referee::Result<DefinitionRecord>::err(edgeR.error->message);
    if (def.migration_hook.has_value()) {
      auto hookProps = referee::cbor_from_json_kv("hook", def.migration_hook.value());
      auto hookR = store_.add_edge(createR.value->ref, priorR.value->value().ref,
                                   "migration_hook", "definition", hookProps);
      if (!hookR) return referee::Result<DefinitionRecord>::err(hookR.error->message);
    }
  } else if (def.migration_hook.has_value()) {
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
