#include "services/service.h"

#include "services/capability_context.h"

#include <chrono>
#include <stdexcept>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace iris::service {

static std::string id_key(const referee::ObjectID& id) {
  return id.to_hex();
}

static bool endpoint_matches(const Endpoint& declared, const Endpoint& requested) {
  if (!declared.name.empty() && !requested.name.empty() && declared.name == requested.name) {
    return true;
  }
  if (declared.type.has_value() && requested.type.has_value() &&
      declared.type.value() == requested.type.value()) {
    return true;
  }
  return false;
}

static std::optional<Endpoint> find_declared_endpoint(const ServiceDescriptor& service,
                                                      const std::optional<Endpoint>& requested) {
  if (!requested.has_value()) return std::nullopt;
  for (const auto& endpoint : service.endpoints) {
    if (endpoint_matches(endpoint, requested.value())) return endpoint;
  }
  return std::nullopt;
}

static std::string endpoint_key(const std::optional<Endpoint>& endpoint) {
  if (!endpoint.has_value()) return {};
  if (!endpoint->name.empty()) return endpoint->name;
  if (endpoint->type.has_value()) {
    if (endpoint->type.value() == kMemoryRegisterRegionType) return "memory.register_region";
    if (endpoint->type.value() == kMemoryListRegionsType) return "memory.list_regions";
    if (endpoint->type.value() == kMemoryLookupRegionType) return "memory.lookup_region";
  }
  return {};
}

MessageEnvelope make_request_to_object(referee::ObjectID sender,
                                       referee::ObjectID recipient,
                                       referee::TypeID message_type,
                                       referee::Bytes payload_cbor,
                                       std::optional<referee::ObjectID> correlation_id) {
  MessageEnvelope env;
  env.sender = sender;
  env.recipient = recipient;
  env.message_type = message_type;
  env.payload_cbor = std::move(payload_cbor);
  env.correlation_id = correlation_id.value_or(referee::ObjectID::random());
  env.timestamp_unix_ms = referee::unix_ms_now();
  return env;
}

MessageEnvelope make_request_to_endpoint(referee::ObjectID sender,
                                         Endpoint endpoint,
                                         referee::TypeID message_type,
                                         referee::Bytes payload_cbor,
                                         std::optional<referee::ObjectID> correlation_id) {
  MessageEnvelope env;
  env.sender = sender;
  env.endpoint = std::move(endpoint);
  env.message_type = message_type;
  env.payload_cbor = std::move(payload_cbor);
  env.correlation_id = correlation_id.value_or(referee::ObjectID::random());
  env.timestamp_unix_ms = referee::unix_ms_now();
  return env;
}

MessageEnvelope make_response(const MessageEnvelope& request,
                              referee::ObjectID responder,
                              referee::TypeID message_type,
                              referee::Bytes payload_cbor) {
  MessageEnvelope env;
  env.sender = responder;
  env.recipient = request.sender;
  env.sandbox = request.sandbox;
  env.message_type = message_type;
  env.payload_cbor = std::move(payload_cbor);
  env.correlation_id = request.correlation_id;
  env.timestamp_unix_ms = referee::unix_ms_now();
  return env;
}

std::string_view memory_region_kind_name(MemoryRegionKind kind) {
  switch (kind) {
  case MemoryRegionKind::Ram:
    return "ram";
  case MemoryRegionKind::Flash:
    return "flash";
  case MemoryRegionKind::ReadOnly:
    return "read_only";
  }
  return "unknown";
}

bool memory_region_is_writable(MemoryRegionKind kind) {
  return kind == MemoryRegionKind::Ram || kind == MemoryRegionKind::Flash;
}

bool memory_region_is_persistent(MemoryRegionKind kind) {
  return kind == MemoryRegionKind::Flash || kind == MemoryRegionKind::ReadOnly;
}

static referee::Result<MemoryRegionKind> memory_region_kind_from_name(std::string_view name) {
  if (name == "ram") return referee::Result<MemoryRegionKind>::ok(MemoryRegionKind::Ram);
  if (name == "flash") return referee::Result<MemoryRegionKind>::ok(MemoryRegionKind::Flash);
  if (name == "read_only") return referee::Result<MemoryRegionKind>::ok(MemoryRegionKind::ReadOnly);
  return referee::Result<MemoryRegionKind>::err(referee::ErrorCode::InvalidArgument,
                                                "unknown memory region kind");
}

static nlohmann::json memory_region_to_json(const MemoryRegion& region) {
  nlohmann::json j;
  j["id"] = region.id.to_hex();
  j["name"] = region.name;
  j["kind"] = std::string(memory_region_kind_name(region.kind));
  j["base"] = region.base;
  j["size"] = region.size;
  j["writable"] = memory_region_is_writable(region.kind);
  j["persistent"] = memory_region_is_persistent(region.kind);
  return j;
}

static referee::Result<MemoryRegion> memory_region_from_json(const nlohmann::json& j) {
  if (!j.is_object()) {
    return referee::Result<MemoryRegion>::err(referee::ErrorCode::InvalidArgument,
                                              "memory region payload must be an object");
  }

  try {
    MemoryRegion region;
    region.id = referee::ObjectID::from_hex(j.at("id").get<std::string>());
    region.name = j.at("name").get<std::string>();
    auto kindR = memory_region_kind_from_name(j.at("kind").get<std::string>());
    if (!kindR) return referee::Result<MemoryRegion>::err(kindR.error.value());
    region.kind = kindR.value.value();
    region.base = j.at("base").get<std::uint64_t>();
    region.size = j.at("size").get<std::uint64_t>();
    return referee::Result<MemoryRegion>::ok(std::move(region));
  } catch (const std::exception& ex) {
    return referee::Result<MemoryRegion>::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
}

static referee::Result<nlohmann::json> json_from_cbor_payload(const referee::Bytes& payload) {
  try {
    return referee::Result<nlohmann::json>::ok(nlohmann::json::from_cbor(payload));
  } catch (const std::exception& ex) {
    return referee::Result<nlohmann::json>::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
}

MemoryService::MemoryService(referee::ObjectID id) {
  desc_.id = id;
  desc_.type = kMemoryServiceType;
  desc_.name = "memory";
  desc_.endpoints.push_back(Endpoint{"memory.register_region", kMemoryRegisterRegionType, {}});
  desc_.endpoints.push_back(Endpoint{"memory.list_regions", kMemoryListRegionsType, {}});
  desc_.endpoints.push_back(Endpoint{"memory.lookup_region", kMemoryLookupRegionType, {}});
}

ServiceDescriptor MemoryService::descriptor() const {
  return desc_;
}

referee::Result<MemoryRegion> MemoryService::register_region(const MemoryRegion& region) {
  if (region.name.empty()) {
    return referee::Result<MemoryRegion>::err(referee::ErrorCode::InvalidArgument,
                                              "memory region name is empty");
  }
  if (region.size == 0) {
    return referee::Result<MemoryRegion>::err(referee::ErrorCode::InvalidArgument,
                                              "memory region size is zero");
  }

  for (const auto& existing : regions_) {
    if (existing.id == region.id) {
      return referee::Result<MemoryRegion>::err(referee::ErrorCode::AlreadyExists,
                                                "memory region id already registered");
    }
    if (existing.name == region.name) {
      return referee::Result<MemoryRegion>::err(referee::ErrorCode::AlreadyExists,
                                                "memory region name already registered");
    }
  }

  regions_.push_back(region);
  return referee::Result<MemoryRegion>::ok(region);
}

referee::Result<std::optional<MemoryRegion>> MemoryService::lookup_region(referee::ObjectID id) const {
  for (const auto& region : regions_) {
    if (region.id == id) {
      return referee::Result<std::optional<MemoryRegion>>::ok(std::optional<MemoryRegion>{region});
    }
  }
  return referee::Result<std::optional<MemoryRegion>>::ok(std::optional<MemoryRegion>{});
}

std::vector<MemoryRegion> MemoryService::list_regions() const {
  return regions_;
}

referee::Result<MessageEnvelope> MemoryService::handle_message(const MessageEnvelope& request) {
  auto key = endpoint_key(request.endpoint);
  if (key == "memory.register_region") {
    auto jsonR = json_from_cbor_payload(request.payload_cbor);
    if (!jsonR) return referee::Result<MessageEnvelope>::err(jsonR.error.value());
    auto regionR = memory_region_from_json(jsonR.value.value());
    if (!regionR) return referee::Result<MessageEnvelope>::err(regionR.error.value());
    auto registeredR = register_region(regionR.value.value());
    if (!registeredR) return referee::Result<MessageEnvelope>::err(registeredR.error.value());
    auto response = make_response(request,
                                  desc_.id,
                                  kMemoryRegionResponseType,
                                  nlohmann::json::to_cbor(memory_region_to_json(registeredR.value.value())));
    return referee::Result<MessageEnvelope>::ok(std::move(response));
  }

  if (key == "memory.list_regions") {
    nlohmann::json root;
    root["regions"] = nlohmann::json::array();
    for (const auto& region : regions_) {
      root["regions"].push_back(memory_region_to_json(region));
    }
    auto response = make_response(request,
                                  desc_.id,
                                  kMemoryRegionListResponseType,
                                  nlohmann::json::to_cbor(root));
    return referee::Result<MessageEnvelope>::ok(std::move(response));
  }

  if (key == "memory.lookup_region") {
    auto jsonR = json_from_cbor_payload(request.payload_cbor);
    if (!jsonR) return referee::Result<MessageEnvelope>::err(jsonR.error.value());

    try {
      auto id = referee::ObjectID::from_hex(jsonR.value->at("id").get<std::string>());
      auto foundR = lookup_region(id);
      if (!foundR) return referee::Result<MessageEnvelope>::err(foundR.error.value());

      nlohmann::json root;
      root["found"] = foundR.value->has_value();
      if (foundR.value->has_value()) {
        root["region"] = memory_region_to_json(foundR.value->value());
      }

      auto response = make_response(request,
                                    desc_.id,
                                    kMemoryRegionResponseType,
                                    nlohmann::json::to_cbor(root));
      return referee::Result<MessageEnvelope>::ok(std::move(response));
    } catch (const std::exception& ex) {
      return referee::Result<MessageEnvelope>::err(referee::ErrorCode::InvalidArgument, ex.what());
    }
  }

  return referee::Result<MessageEnvelope>::err(referee::ErrorCode::InvalidArgument,
                                               "unsupported memory service endpoint");
}

referee::Result<void> ServiceRegistry::register_service(const ServiceDescriptor& desc,
                                                        ServiceObject* handler) {
  if (!handler) return referee::Result<void>::err("handler is null");

  auto key = id_key(desc.id);
  if (by_id_.find(key) != by_id_.end()) return referee::Result<void>::err("service id already registered");

  if (!desc.name.empty()) {
    auto name_it = by_name_.find(desc.name);
    if (name_it != by_name_.end()) return referee::Result<void>::err("service name already registered");
  }

  auto type_it = by_type_.find(desc.type.v);
  if (type_it != by_type_.end()) return referee::Result<void>::err("service type already registered");

  by_id_.emplace(key, Entry{desc, handler});
  if (!desc.name.empty()) by_name_[desc.name] = key;
  by_type_[desc.type.v] = key;

  return referee::Result<void>::ok();
}

referee::Result<void> ServiceRegistry::unregister_service(const referee::ObjectID& id) {
  auto key = id_key(id);
  auto it = by_id_.find(key);
  if (it == by_id_.end()) return referee::Result<void>::err("service id not registered");

  if (!it->second.desc.name.empty()) {
    auto name_it = by_name_.find(it->second.desc.name);
    if (name_it != by_name_.end()) by_name_.erase(name_it);
  }

  auto type_it = by_type_.find(it->second.desc.type.v);
  if (type_it != by_type_.end()) by_type_.erase(type_it);

  by_id_.erase(it);
  return referee::Result<void>::ok();
}

referee::Result<std::optional<ServiceDescriptor>> ServiceRegistry::resolve_by_name(std::string_view name) const {
  if (name.empty()) return referee::Result<std::optional<ServiceDescriptor>>::err("service name is empty");

  auto name_it = by_name_.find(std::string(name));
  if (name_it == by_name_.end()) {
    return referee::Result<std::optional<ServiceDescriptor>>::ok(std::optional<ServiceDescriptor>{});
  }

  auto id_it = by_id_.find(name_it->second);
  if (id_it == by_id_.end()) {
    return referee::Result<std::optional<ServiceDescriptor>>::err("registry corrupted for name");
  }

  return referee::Result<std::optional<ServiceDescriptor>>::ok(id_it->second.desc);
}

referee::Result<std::optional<ServiceDescriptor>> ServiceRegistry::resolve_by_type(referee::TypeID type) const {
  auto type_it = by_type_.find(type.v);
  if (type_it == by_type_.end()) {
    return referee::Result<std::optional<ServiceDescriptor>>::ok(std::optional<ServiceDescriptor>{});
  }

  auto id_it = by_id_.find(type_it->second);
  if (id_it == by_id_.end()) {
    return referee::Result<std::optional<ServiceDescriptor>>::err("registry corrupted for type");
  }

  return referee::Result<std::optional<ServiceDescriptor>>::ok(id_it->second.desc);
}

ServiceObject* ServiceRegistry::handler_for(const referee::ObjectID& id) const {
  auto it = by_id_.find(id_key(id));
  if (it == by_id_.end()) return nullptr;
  return it->second.handler;
}

CapabilityContextAuthorizer::CapabilityContextAuthorizer(CapabilityContextStore& contexts)
    : contexts_(contexts) {}

referee::Result<void> CapabilityContextAuthorizer::authorize(
    const MessageEnvelope& request,
    const ServiceDescriptor& service,
    const std::optional<Endpoint>& endpoint) {
  std::vector<std::string> required;
  required.reserve(service.required_grants.size() +
                   (endpoint.has_value() ? endpoint->required_grants.size() : 0));
  for (const auto& grant : service.required_grants) required.push_back(grant);
  if (endpoint.has_value()) {
    for (const auto& grant : endpoint->required_grants) required.push_back(grant);
  }

  if (required.empty()) return referee::Result<void>::ok();

  auto contextsR = contexts_.list_contexts_for_subject(request.sender);
  if (!contextsR) return referee::Result<void>::err(contextsR.error.value());

  std::unordered_set<std::string> granted;
  for (const auto& record : contextsR.value.value()) {
    for (const auto& grant : record.context.grants) {
      granted.insert(grant.name);
    }
  }

  for (const auto& grant : required) {
    if (granted.find(grant) == granted.end()) {
      return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                        "missing capability grant: " + grant);
    }
  }

  return referee::Result<void>::ok();
}

IpcService::IpcService(ServiceRegistry& registry) : registry_(registry) {}

IpcService::IpcService(ServiceRegistry& registry, ServiceBoundaryAuthorizer* authorizer)
    : registry_(registry), authorizer_(authorizer) {}

std::optional<IpcService::ResolvedService> IpcService::resolve_service(
    const MessageEnvelope& request) const {
  if (request.recipient.has_value()) {
    auto* handler = registry_.handler_for(request.recipient.value());
    if (!handler) return std::nullopt;
    auto descriptor = handler->descriptor();
    return ResolvedService{descriptor, find_declared_endpoint(descriptor, request.endpoint), handler};
  }
  if (!request.endpoint.has_value()) return std::nullopt;

  const auto& endpoint = request.endpoint.value();
  if (!endpoint.name.empty()) {
    auto resolved = registry_.resolve_by_name(endpoint.name);
    if (!resolved || !resolved.value->has_value()) return std::nullopt;
    const auto& descriptor = resolved.value->value();
    auto* handler = registry_.handler_for(descriptor.id);
    if (!handler) return std::nullopt;
    return ResolvedService{descriptor, find_declared_endpoint(descriptor, request.endpoint), handler};
  }

  if (endpoint.type.has_value()) {
    auto resolved = registry_.resolve_by_type(endpoint.type.value());
    if (!resolved || !resolved.value->has_value()) return std::nullopt;
    const auto& descriptor = resolved.value->value();
    auto* handler = registry_.handler_for(descriptor.id);
    if (!handler) return std::nullopt;
    return ResolvedService{descriptor, find_declared_endpoint(descriptor, request.endpoint), handler};
  }

  return std::nullopt;
}

referee::Result<MessageEnvelope> IpcService::send_request(const MessageEnvelope& request,
                                                          std::chrono::milliseconds timeout) {
  if (timeout.count() <= 0) return referee::Result<MessageEnvelope>::err("timeout");

  auto resolved = resolve_service(request);
  if (!resolved.has_value()) return referee::Result<MessageEnvelope>::err("service not found");

  if (authorizer_) {
    auto authR = authorizer_->authorize(request, resolved->descriptor, resolved->endpoint);
    if (!authR) return referee::Result<MessageEnvelope>::err(authR.error.value());
  }

  auto start = std::chrono::steady_clock::now();
  auto response = resolved->handler->handle_message(request);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

  if (elapsed > timeout) return referee::Result<MessageEnvelope>::err("timeout");
  if (!response) return response;
  if (!response.value.has_value()) return referee::Result<MessageEnvelope>::err("empty response");

  if (response.value->correlation_id != request.correlation_id) {
    return referee::Result<MessageEnvelope>::err("correlation id mismatch");
  }

  return response;
}

} // namespace iris::service
