#include "comms/transport_session.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace iris::comms {

namespace {

bool descriptor_exists(const machine::MachineInventory& inventory,
                       machine::MachineResourceKind resource_kind,
                       referee::ObjectID resource_id) {
  switch (resource_kind) {
    case machine::MachineResourceKind::Memory:
      return inventory.find_available_memory_block(resource_id) != nullptr;
    case machine::MachineResourceKind::Device:
      return inventory.find_device(resource_id) != nullptr;
    case machine::MachineResourceKind::IoRegion: {
      const machine::MemoryRegion* region = inventory.find_memory_region(resource_id);
      return region != nullptr && region->kind == machine::MemoryRegionKind::MemoryMapped;
    }
  }
  return false;
}

machine::MachineResourceKind handle_resource_kind(const machine::MachineHandle& handle) {
  return std::visit([](const auto& typed_handle) {
    using Handle = std::decay_t<decltype(typed_handle)>;
    if constexpr (std::is_same_v<Handle, machine::MemoryHandle>) {
      return machine::MachineResourceKind::Memory;
    }
    if constexpr (std::is_same_v<Handle, machine::DeviceHandle>) {
      return machine::MachineResourceKind::Device;
    }
    return machine::MachineResourceKind::IoRegion;
  }, handle);
}

} // namespace

referee::Result<Transport> TransportFactory::create_descriptor_transport(
    referee::ObjectID transport_id,
    TransportSemantics semantics,
    machine::MachineResourceKind resource_kind,
    referee::ObjectID resource_id) const {
  if (!descriptor_exists(inventory_, resource_kind, resource_id)) {
    return referee::Result<Transport>::err(
        referee::ErrorCode::NotFound, "Machine transport resource not found");
  }
  DescriptorTransportResource resource{resource_kind, resource_id};
  return referee::Result<Transport>::ok(
      Transport(transport_id, semantics, TransportResource{resource}));
}

referee::Result<Transport> TransportFactory::create_authorized_transport(
    referee::ObjectID transport_id,
    TransportSemantics semantics,
    referee::ObjectID lease_id,
    referee::ObjectID owner_id,
    referee::ObjectID capability_context_id) const {
  auto authorization = leases_.authorize_use(lease_id, owner_id, capability_context_id);
  if (!authorization) {
    return referee::Result<Transport>::err(authorization.error.value());
  }
  AuthorizedTransportResource resource{lease_id, owner_id, capability_context_id};
  return referee::Result<Transport>::ok(
      Transport(transport_id, semantics, TransportResource{resource}));
}

referee::Result<void> Session::open() {
  return transition(SessionState::Created, SessionState::Open);
}

referee::Result<void> Session::begin_close() {
  return transition(SessionState::Open, SessionState::Closing);
}

referee::Result<void> Session::finish_close() {
  return transition(SessionState::Closing, SessionState::Closed);
}

referee::Result<void> Session::transition(SessionState expected, SessionState next) {
  if (state_ != expected) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "invalid Comms session transition");
  }
  state_ = next;
  return referee::Result<void>::ok();
}

referee::Result<Protocol> Protocol::create(
    referee::ObjectID id,
    std::string name,
    TransportSemantics required_semantics,
    std::vector<machine::MachineResourceKind> allowed_resource_kinds) {
  if (name.empty()) {
    return referee::Result<Protocol>::err(referee::ErrorCode::InvalidArgument,
                                          "protocol name must not be empty");
  }
  if (allowed_resource_kinds.empty()) {
    return referee::Result<Protocol>::err(
        referee::ErrorCode::InvalidArgument,
        "protocol must allow at least one Machine resource kind");
  }
  std::sort(allowed_resource_kinds.begin(), allowed_resource_kinds.end());
  allowed_resource_kinds.erase(
      std::unique(allowed_resource_kinds.begin(), allowed_resource_kinds.end()),
      allowed_resource_kinds.end());
  return referee::Result<Protocol>::ok(
      Protocol(id, std::move(name), required_semantics,
               std::move(allowed_resource_kinds)));
}

CompatibilityResult check_compatibility(
    const Protocol& protocol,
    const Transport& transport,
    const machine::MachineLeaseRegistry& leases) {
  if (protocol.required_semantics() != transport.semantics()) {
    return {false, CompatibilityReason::TransportSemanticsMismatch, std::nullopt};
  }

  std::optional<machine::MachineResourceKind> resource_kind;
  if (const auto* descriptor = std::get_if<DescriptorTransportResource>(
          &transport.resource())) {
    resource_kind = descriptor->resource_kind;
  } else {
    const auto& authorized = std::get<AuthorizedTransportResource>(transport.resource());
    auto authorization = leases.authorize_use(
        authorized.lease_id, authorized.owner_id, authorized.capability_context_id);
    if (!authorization) {
      return {false, CompatibilityReason::LeaseAuthorizationFailed, std::nullopt};
    }
    const machine::MachineLease* lease = leases.find(authorized.lease_id);
    if (lease == nullptr) {
      return {false, CompatibilityReason::LeaseAuthorizationFailed, std::nullopt};
    }
    resource_kind = handle_resource_kind(lease->handle());
  }

  const auto& allowed = protocol.allowed_resource_kinds();
  if (std::find(allowed.begin(), allowed.end(), *resource_kind) == allowed.end()) {
    return {false, CompatibilityReason::ResourceKindNotAllowed, resource_kind};
  }
  return {true, CompatibilityReason::Compatible, resource_kind};
}

referee::Result<Bytes> encode_frame(const machine::Packet& packet) {
  if (packet.size() > kMaximumFramePayload) {
    return referee::Result<Bytes>::err(referee::ErrorCode::InvalidArgument,
                                       "frame payload exceeds 1 MiB maximum");
  }

  const auto length = static_cast<std::uint32_t>(packet.size());
  Bytes frame;
  frame.reserve(4U + packet.size());
  frame.push_back(static_cast<std::uint8_t>((length >> 24U) & 0xffU));
  frame.push_back(static_cast<std::uint8_t>((length >> 16U) & 0xffU));
  frame.push_back(static_cast<std::uint8_t>((length >> 8U) & 0xffU));
  frame.push_back(static_cast<std::uint8_t>(length & 0xffU));
  for (machine::Byte byte : packet.payload().bytes()) frame.push_back(byte.value());
  return referee::Result<Bytes>::ok(std::move(frame));
}

referee::Result<machine::Packet> decode_frame(const Bytes& frame) {
  if (frame.size() < 4U) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::CorruptData,
                                                 "frame header is truncated");
  }
  const std::uint32_t declared_length =
      (static_cast<std::uint32_t>(frame[0]) << 24U)
      | (static_cast<std::uint32_t>(frame[1]) << 16U)
      | (static_cast<std::uint32_t>(frame[2]) << 8U)
      | static_cast<std::uint32_t>(frame[3]);
  if (declared_length > kMaximumFramePayload) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::CorruptData,
                                                 "declared payload exceeds 1 MiB maximum");
  }
  const std::size_t expected_size = 4U + static_cast<std::size_t>(declared_length);
  if (frame.size() < expected_size) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::CorruptData,
                                                 "frame payload is truncated");
  }
  if (frame.size() > expected_size) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::CorruptData,
                                                 "frame has trailing bytes");
  }

  std::vector<machine::Byte> payload;
  payload.reserve(declared_length);
  for (std::size_t index = 4U; index < frame.size(); ++index) {
    payload.emplace_back(frame[index]);
  }
  return referee::Result<machine::Packet>::ok(
      machine::Packet(machine::Blob(std::move(payload))));
}

referee::Result<machine::Packet> execute_registered_packet_round_trip(
    refract::SchemaRegistry& schemas,
    const machine::MachineLeaseRegistry& leases,
    const Protocol& protocol,
    const Transport& transport,
    const Session& session,
    const machine::Packet& packet) {
  auto packet_definition = schemas.get_definition_by_type(machine::kPacketType);
  if (!packet_definition) {
    return referee::Result<machine::Packet>::err(packet_definition.error.value());
  }
  if (!packet_definition.value->has_value()) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::FailedPrecondition,
                                                 "Machine::Packet is not registered");
  }
  if (session.state() != SessionState::Open) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::FailedPrecondition,
                                                 "Comms session is not open");
  }
  if (session.transport_id() != transport.id()) {
    return referee::Result<machine::Packet>::err(
        referee::ErrorCode::FailedPrecondition,
        "Comms session does not reference the supplied transport");
  }
  if (protocol.required_semantics() != TransportSemantics::Stream
      || transport.semantics() != TransportSemantics::Stream) {
    return referee::Result<machine::Packet>::err(
        referee::ErrorCode::FailedPrecondition,
        "registered packet execution requires stream semantics");
  }
  const CompatibilityResult compatibility = check_compatibility(protocol, transport, leases);
  if (!compatibility.compatible) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::FailedPrecondition,
                                                 "protocol transport is incompatible");
  }

  auto encoded = encode_frame(packet);
  if (!encoded) return referee::Result<machine::Packet>::err(encoded.error.value());
  auto channels = Channel::loopback();
  if (!channels.first.send(*encoded.value).ready) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::IoError,
                                                 "loopback frame send failed");
  }
  Bytes received = channels.second.recv(channels.second.available());
  auto decoded = decode_frame(received);
  if (!decoded) return decoded;
  if (*decoded.value != packet) {
    return referee::Result<machine::Packet>::err(referee::ErrorCode::CorruptData,
                                                 "packet round-trip equality check failed");
  }
  return decoded;
}

} // namespace iris::comms
