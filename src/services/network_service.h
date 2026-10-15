#pragma once

#include "comms/transport_session.h"
#include "services/service.h"

#include <map>

namespace iris::service {

constexpr referee::TypeID kNetworkServiceType{0x5356434E45540001ULL};
constexpr referee::TypeID kNetworkOpenType{0x5356434E45541001ULL};
constexpr referee::TypeID kNetworkRoundTripType{0x5356434E45541002ULL};
constexpr referee::TypeID kNetworkCloseType{0x5356434E45541003ULL};
constexpr referee::TypeID kNetworkInspectType{0x5356434E45541004ULL};
constexpr referee::TypeID kNetworkResponseType{0x5356434E45542001ULL};

// Serialized local loopback service. Owns immutable protocol/transport bindings and sessions.
// Schemas and leases (including its inventory/context inputs) must outlive this handler.
// Closed records remain inspectable and count toward maximum_sessions until handler destruction.
class NetworkService final : public ServiceObject {
public:
  NetworkService(referee::ObjectID id, refract::SchemaRegistry& schemas,
                 const machine::MachineLeaseRegistry& leases, comms::Protocol protocol,
                 comms::Transport transport, std::size_t maximum_sessions);
  NetworkService(const NetworkService&) = delete;
  NetworkService& operator=(const NetworkService&) = delete;

  ServiceDescriptor descriptor() const override { return desc_; }
  referee::Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override;

private:
  referee::Result<void> authorize(referee::ObjectID sender) const;
  ServiceDescriptor desc_;
  refract::SchemaRegistry& schemas_;
  const machine::MachineLeaseRegistry& leases_;
  comms::Protocol protocol_;
  comms::Transport transport_;
  std::size_t maximum_sessions_;
  std::map<std::string, comms::Session> sessions_;
};

} // namespace iris::service
