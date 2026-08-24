#include "comms/transport_session.h"

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

} // namespace iris::comms
