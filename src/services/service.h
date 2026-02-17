#pragma once

#include "referee/referee.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace iris::service {

struct Endpoint {
  std::string name;
  std::optional<referee::TypeID> type;
};

struct MessageEnvelope {
  referee::ObjectID sender{};
  std::optional<referee::ObjectID> recipient{};
  std::optional<Endpoint> endpoint{};
  referee::TypeID message_type{};
  referee::Bytes payload_cbor{};
  referee::ObjectID correlation_id{};
  std::uint64_t timestamp_unix_ms{};
};

MessageEnvelope make_request_to_object(referee::ObjectID sender,
                                       referee::ObjectID recipient,
                                       referee::TypeID message_type,
                                       referee::Bytes payload_cbor,
                                       std::optional<referee::ObjectID> correlation_id = std::nullopt);

MessageEnvelope make_request_to_endpoint(referee::ObjectID sender,
                                         Endpoint endpoint,
                                         referee::TypeID message_type,
                                         referee::Bytes payload_cbor,
                                         std::optional<referee::ObjectID> correlation_id = std::nullopt);

MessageEnvelope make_response(const MessageEnvelope& request,
                              referee::ObjectID responder,
                              referee::TypeID message_type,
                              referee::Bytes payload_cbor);

struct ServiceDescriptor {
  referee::ObjectID id{};
  referee::TypeID type{};
  std::string name;
  std::vector<Endpoint> endpoints;
};

class ServiceObject {
public:
  virtual ~ServiceObject() = default;
  virtual ServiceDescriptor descriptor() const = 0;
  virtual referee::Result<MessageEnvelope> handle_message(const MessageEnvelope& request) = 0;
};

class ServiceRegistry {
public:
  referee::Result<void> register_service(const ServiceDescriptor& desc, ServiceObject* handler);
  referee::Result<void> unregister_service(const referee::ObjectID& id);

  referee::Result<std::optional<ServiceDescriptor>> resolve_by_name(std::string_view name) const;
  referee::Result<std::optional<ServiceDescriptor>> resolve_by_type(referee::TypeID type) const;

  ServiceObject* handler_for(const referee::ObjectID& id) const;

private:
  struct Entry {
    ServiceDescriptor desc;
    ServiceObject* handler{nullptr};
  };

  std::unordered_map<std::string, Entry> by_id_;
  std::unordered_map<std::string, std::string> by_name_;
  std::unordered_map<std::uint64_t, std::string> by_type_;
};

class IpcService {
public:
  explicit IpcService(ServiceRegistry& registry);

  referee::Result<MessageEnvelope> send_request(const MessageEnvelope& request,
                                                std::chrono::milliseconds timeout);

private:
  ServiceObject* resolve_handler(const MessageEnvelope& request) const;

  ServiceRegistry& registry_;
};

} // namespace iris::service
