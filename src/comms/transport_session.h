#pragma once

#include "machine/authority.h"

#include <utility>
#include <variant>

namespace iris::comms {

enum class TransportSemantics {
  Stream,
  Datagram
};

struct DescriptorTransportResource {
  machine::MachineResourceKind resource_kind{machine::MachineResourceKind::Device};
  referee::ObjectID resource_id{};
};

struct AuthorizedTransportResource {
  referee::ObjectID lease_id{};
  referee::ObjectID owner_id{};
  referee::ObjectID capability_context_id{};
};

using TransportResource = std::variant<DescriptorTransportResource,
                                       AuthorizedTransportResource>;

class Transport {
public:
  const referee::ObjectID& id() const { return id_; }
  TransportSemantics semantics() const { return semantics_; }
  const TransportResource& resource() const { return resource_; }

private:
  friend class TransportFactory;

  Transport(referee::ObjectID id,
            TransportSemantics semantics,
            TransportResource resource)
      : id_(id), semantics_(semantics), resource_(std::move(resource)) {}

  referee::ObjectID id_{};
  TransportSemantics semantics_{TransportSemantics::Stream};
  TransportResource resource_{};
};

class TransportFactory {
public:
  TransportFactory(const machine::MachineInventory& inventory,
                   const machine::MachineLeaseRegistry& leases)
      : inventory_(inventory), leases_(leases) {}

  referee::Result<Transport> create_descriptor_transport(
      referee::ObjectID transport_id,
      TransportSemantics semantics,
      machine::MachineResourceKind resource_kind,
      referee::ObjectID resource_id) const;

  referee::Result<Transport> create_authorized_transport(
      referee::ObjectID transport_id,
      TransportSemantics semantics,
      referee::ObjectID lease_id,
      referee::ObjectID owner_id,
      referee::ObjectID capability_context_id) const;

private:
  const machine::MachineInventory& inventory_;
  const machine::MachineLeaseRegistry& leases_;
};

struct Endpoint {
  referee::ObjectID id{};
};

enum class SessionState {
  Created,
  Open,
  Closing,
  Closed
};

class Session {
public:
  Session(referee::ObjectID id,
          Endpoint first_endpoint,
          Endpoint second_endpoint,
          referee::ObjectID transport_id)
      : id_(id),
        first_endpoint_id_(first_endpoint.id),
        second_endpoint_id_(second_endpoint.id),
        transport_id_(transport_id) {}

  const referee::ObjectID& id() const { return id_; }
  const referee::ObjectID& first_endpoint_id() const { return first_endpoint_id_; }
  const referee::ObjectID& second_endpoint_id() const { return second_endpoint_id_; }
  const referee::ObjectID& transport_id() const { return transport_id_; }
  SessionState state() const { return state_; }

  referee::Result<void> open();
  referee::Result<void> begin_close();
  referee::Result<void> finish_close();

private:
  referee::Result<void> transition(SessionState expected, SessionState next);

  referee::ObjectID id_{};
  referee::ObjectID first_endpoint_id_{};
  referee::ObjectID second_endpoint_id_{};
  referee::ObjectID transport_id_{};
  SessionState state_{SessionState::Created};
};

} // namespace iris::comms
