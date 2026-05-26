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

class CapabilityContextStore;

struct Endpoint {
  std::string name;
  std::optional<referee::TypeID> type;
  std::vector<std::string> required_grants;
};

struct MessageEnvelope {
  referee::ObjectID sender{};
  std::optional<referee::ObjectID> recipient{};
  std::optional<Endpoint> endpoint{};
  std::optional<referee::ObjectID> sandbox{};
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
  std::vector<std::string> required_grants;
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

class ServiceBoundaryAuthorizer {
public:
  virtual ~ServiceBoundaryAuthorizer() = default;
  virtual referee::Result<void> authorize(const MessageEnvelope& request,
                                          const ServiceDescriptor& service,
                                          const std::optional<Endpoint>& endpoint) = 0;
};

class CapabilityContextAuthorizer final : public ServiceBoundaryAuthorizer {
public:
  explicit CapabilityContextAuthorizer(CapabilityContextStore& contexts);

  referee::Result<void> authorize(const MessageEnvelope& request,
                                  const ServiceDescriptor& service,
                                  const std::optional<Endpoint>& endpoint) override;

private:
  CapabilityContextStore& contexts_;
};

class IpcService {
public:
  explicit IpcService(ServiceRegistry& registry);
  IpcService(ServiceRegistry& registry, ServiceBoundaryAuthorizer* authorizer);

  referee::Result<MessageEnvelope> send_request(const MessageEnvelope& request,
                                                std::chrono::milliseconds timeout);

private:
  struct ResolvedService {
    ServiceDescriptor descriptor{};
    std::optional<Endpoint> endpoint;
    ServiceObject* handler{nullptr};
  };

  std::optional<ResolvedService> resolve_service(const MessageEnvelope& request) const;

  ServiceRegistry& registry_;
  ServiceBoundaryAuthorizer* authorizer_{nullptr};
};

enum class MemoryRegionKind {
  Ram,
  Flash,
  ReadOnly,
};

struct MemoryRegion {
  referee::ObjectID id{};
  std::string name;
  MemoryRegionKind kind{MemoryRegionKind::Ram};
  std::uint64_t base{};
  std::uint64_t size{};
};

constexpr referee::TypeID kMemoryServiceType{0x5356434D454D0001ULL};
constexpr referee::TypeID kMemoryRegisterRegionType{0x5356434D454D1001ULL};
constexpr referee::TypeID kMemoryListRegionsType{0x5356434D454D1002ULL};
constexpr referee::TypeID kMemoryLookupRegionType{0x5356434D454D1003ULL};
constexpr referee::TypeID kMemoryRegionResponseType{0x5356434D454D2001ULL};
constexpr referee::TypeID kMemoryRegionListResponseType{0x5356434D454D2002ULL};

std::string_view memory_region_kind_name(MemoryRegionKind kind);
bool memory_region_is_writable(MemoryRegionKind kind);
bool memory_region_is_persistent(MemoryRegionKind kind);

class MemoryService final : public ServiceObject {
public:
  explicit MemoryService(referee::ObjectID id);

  ServiceDescriptor descriptor() const override;
  referee::Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override;

  referee::Result<MemoryRegion> register_region(const MemoryRegion& region);
  referee::Result<std::optional<MemoryRegion>> lookup_region(referee::ObjectID id) const;
  std::vector<MemoryRegion> list_regions() const;

private:
  ServiceDescriptor desc_{};
  std::vector<MemoryRegion> regions_;
};

} // namespace iris::service
