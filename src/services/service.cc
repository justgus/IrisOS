#include "services/service.h"

#include <chrono>

namespace iris::service {

static std::string id_key(const referee::ObjectID& id) {
  return id.to_hex();
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
  env.message_type = message_type;
  env.payload_cbor = std::move(payload_cbor);
  env.correlation_id = request.correlation_id;
  env.timestamp_unix_ms = referee::unix_ms_now();
  return env;
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

IpcService::IpcService(ServiceRegistry& registry) : registry_(registry) {}

ServiceObject* IpcService::resolve_handler(const MessageEnvelope& request) const {
  if (request.recipient.has_value()) return registry_.handler_for(request.recipient.value());
  if (!request.endpoint.has_value()) return nullptr;

  const auto& endpoint = request.endpoint.value();
  if (!endpoint.name.empty()) {
    auto resolved = registry_.resolve_by_name(endpoint.name);
    if (!resolved || !resolved.value->has_value()) return nullptr;
    return registry_.handler_for(resolved.value->value().id);
  }

  if (endpoint.type.has_value()) {
    auto resolved = registry_.resolve_by_type(endpoint.type.value());
    if (!resolved || !resolved.value->has_value()) return nullptr;
    return registry_.handler_for(resolved.value->value().id);
  }

  return nullptr;
}

referee::Result<MessageEnvelope> IpcService::send_request(const MessageEnvelope& request,
                                                          std::chrono::milliseconds timeout) {
  if (timeout.count() <= 0) return referee::Result<MessageEnvelope>::err("timeout");

  auto* handler = resolve_handler(request);
  if (!handler) return referee::Result<MessageEnvelope>::err("service not found");

  auto start = std::chrono::steady_clock::now();
  auto response = handler->handle_message(request);
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
