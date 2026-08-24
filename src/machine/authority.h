#pragma once

#include "machine/inventory.h"
#include "services/capability_context.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace iris::machine {

enum class MachineAccessMode {
  Read,
  Write,
  Control
};

enum class MachineResourceKind {
  Memory,
  Device,
  IoRegion
};

struct MemoryHandle {
  referee::ObjectID resource_id{};
  MachineAccessMode access_mode{MachineAccessMode::Read};
  referee::ObjectID capability_context_id{};
};

struct DeviceHandle {
  referee::ObjectID resource_id{};
  MachineAccessMode access_mode{MachineAccessMode::Read};
  referee::ObjectID capability_context_id{};
};

struct IoRegionHandle {
  referee::ObjectID resource_id{};
  MachineAccessMode access_mode{MachineAccessMode::Read};
  referee::ObjectID capability_context_id{};
};

using MachineHandle = std::variant<MemoryHandle, DeviceHandle, IoRegionHandle>;

std::string machine_capability_grant(MachineResourceKind resource_kind,
                                     MachineAccessMode access_mode,
                                     referee::ObjectID resource_id);

class MachineHandleFactory {
public:
  MachineHandleFactory(const MachineInventory& inventory,
                       service::CapabilityContextStore& contexts)
      : inventory_(inventory), contexts_(contexts) {}

  referee::Result<MemoryHandle> create_memory_handle(
      referee::ObjectID resource_id,
      MachineAccessMode access_mode,
      referee::ObjectID capability_context_id);
  referee::Result<DeviceHandle> create_device_handle(
      referee::ObjectID resource_id,
      MachineAccessMode access_mode,
      referee::ObjectID capability_context_id);
  referee::Result<IoRegionHandle> create_io_region_handle(
      referee::ObjectID resource_id,
      MachineAccessMode access_mode,
      referee::ObjectID capability_context_id);

private:
  referee::Result<service::CapabilityContext> load_context(
      referee::ObjectID capability_context_id);

  const MachineInventory& inventory_;
  service::CapabilityContextStore& contexts_;
};

enum class MachineLeaseState {
  Active,
  Released,
  Revoked
};

class MachineLease {
public:
  const referee::ObjectID& lease_id() const { return lease_id_; }
  const referee::ObjectID& owner_id() const { return owner_id_; }
  const referee::ObjectID& capability_context_id() const { return capability_context_id_; }
  const MachineHandle& handle() const { return handle_; }
  MachineLeaseState state() const { return state_; }

private:
  friend class MachineLeaseRegistry;

  MachineLease(referee::ObjectID lease_id,
               referee::ObjectID owner_id,
               referee::ObjectID capability_context_id,
               MachineHandle handle)
      : lease_id_(lease_id),
        owner_id_(owner_id),
        capability_context_id_(capability_context_id),
        handle_(std::move(handle)) {}

  referee::ObjectID lease_id_{};
  referee::ObjectID owner_id_{};
  referee::ObjectID capability_context_id_{};
  MachineHandle handle_{};
  MachineLeaseState state_{MachineLeaseState::Active};
};

class MachineLeaseRegistry {
public:
  explicit MachineLeaseRegistry(service::CapabilityContextStore& contexts)
      : contexts_(contexts) {}

  referee::Result<MachineLease> create_lease(
      referee::ObjectID lease_id,
      referee::ObjectID owner_id,
      MachineHandle handle);

  const MachineLease* find(referee::ObjectID lease_id) const;
  referee::Result<void> authorize_use(referee::ObjectID lease_id,
                                      referee::ObjectID owner_id,
                                      referee::ObjectID capability_context_id) const;
  referee::Result<void> release(referee::ObjectID lease_id, referee::ObjectID owner_id);
  referee::Result<void> revoke(referee::ObjectID lease_id);
  std::size_t revoke_for_owner(referee::ObjectID owner_id);
  std::size_t revoke_for_context(referee::ObjectID capability_context_id);

private:
  MachineLease* find_mutable(referee::ObjectID lease_id);
  service::CapabilityContextStore& contexts_;
  std::vector<MachineLease> leases_;
};

} // namespace iris::machine
