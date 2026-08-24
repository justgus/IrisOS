#include "machine/authority.h"

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <utility>

namespace iris::machine {

namespace {

std::string_view access_mode_name(MachineAccessMode access_mode) {
  switch (access_mode) {
    case MachineAccessMode::Read: return "read";
    case MachineAccessMode::Write: return "write";
    case MachineAccessMode::Control: return "control";
  }
  return "";
}

std::string_view resource_kind_name(MachineResourceKind resource_kind) {
  switch (resource_kind) {
    case MachineResourceKind::Memory: return "memory";
    case MachineResourceKind::Device: return "device";
    case MachineResourceKind::IoRegion: return "io-region";
  }
  return "";
}

bool has_grant(const service::CapabilityContext& context, const std::string& required) {
  return std::any_of(context.grants.begin(), context.grants.end(),
                     [&required](const service::CapabilityGrant& grant) {
                       return grant.name == required;
                     });
}

referee::Result<void> authorize_handle(const service::CapabilityContext& context,
                                       MachineResourceKind resource_kind,
                                       MachineAccessMode access_mode,
                                       referee::ObjectID resource_id) {
  const std::string required = machine_capability_grant(resource_kind, access_mode, resource_id);
  if (!has_grant(context, required)) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "missing capability grant: " + required);
  }
  return referee::Result<void>::ok();
}

referee::ObjectID handle_context_id(const MachineHandle& handle) {
  return std::visit([](const auto& typed_handle) {
    return typed_handle.capability_context_id;
  }, handle);
}

referee::ObjectID handle_resource_id(const MachineHandle& handle) {
  return std::visit([](const auto& typed_handle) {
    return typed_handle.resource_id;
  }, handle);
}

MachineAccessMode handle_access_mode(const MachineHandle& handle) {
  return std::visit([](const auto& typed_handle) {
    return typed_handle.access_mode;
  }, handle);
}

MachineResourceKind handle_resource_kind(const MachineHandle& handle) {
  return std::visit([](const auto& typed_handle) {
    using Handle = std::decay_t<decltype(typed_handle)>;
    if constexpr (std::is_same_v<Handle, MemoryHandle>) return MachineResourceKind::Memory;
    if constexpr (std::is_same_v<Handle, DeviceHandle>) return MachineResourceKind::Device;
    return MachineResourceKind::IoRegion;
  }, handle);
}

} // namespace

std::string machine_capability_grant(MachineResourceKind resource_kind,
                                     MachineAccessMode access_mode,
                                     referee::ObjectID resource_id) {
  return "machine." + std::string(resource_kind_name(resource_kind)) + "." +
         std::string(access_mode_name(access_mode)) + ":" + resource_id.to_hex();
}

referee::Result<MemoryHandle> MachineHandleFactory::create_memory_handle(
    referee::ObjectID resource_id,
    MachineAccessMode access_mode,
    referee::ObjectID capability_context_id) {
  auto context = load_context(capability_context_id);
  if (!context) return referee::Result<MemoryHandle>::err(context.error.value());
  auto authorization = authorize_handle(*context.value, MachineResourceKind::Memory,
                                        access_mode, resource_id);
  if (!authorization) return referee::Result<MemoryHandle>::err(authorization.error.value());
  if (inventory_.find_available_memory_block(resource_id) == nullptr) {
    return referee::Result<MemoryHandle>::err(referee::ErrorCode::NotFound,
                                              "available memory block not found");
  }
  return referee::Result<MemoryHandle>::ok({resource_id, access_mode, capability_context_id});
}

referee::Result<DeviceHandle> MachineHandleFactory::create_device_handle(
    referee::ObjectID resource_id,
    MachineAccessMode access_mode,
    referee::ObjectID capability_context_id) {
  auto context = load_context(capability_context_id);
  if (!context) return referee::Result<DeviceHandle>::err(context.error.value());
  auto authorization = authorize_handle(*context.value, MachineResourceKind::Device,
                                        access_mode, resource_id);
  if (!authorization) return referee::Result<DeviceHandle>::err(authorization.error.value());
  if (inventory_.find_device(resource_id) == nullptr) {
    return referee::Result<DeviceHandle>::err(referee::ErrorCode::NotFound,
                                              "device not found");
  }
  return referee::Result<DeviceHandle>::ok({resource_id, access_mode, capability_context_id});
}

referee::Result<IoRegionHandle> MachineHandleFactory::create_io_region_handle(
    referee::ObjectID resource_id,
    MachineAccessMode access_mode,
    referee::ObjectID capability_context_id) {
  auto context = load_context(capability_context_id);
  if (!context) return referee::Result<IoRegionHandle>::err(context.error.value());
  auto authorization = authorize_handle(*context.value, MachineResourceKind::IoRegion,
                                        access_mode, resource_id);
  if (!authorization) return referee::Result<IoRegionHandle>::err(authorization.error.value());
  const MemoryRegion* region = inventory_.find_memory_region(resource_id);
  if (region == nullptr) {
    return referee::Result<IoRegionHandle>::err(referee::ErrorCode::NotFound,
                                                "memory region not found");
  }
  if (region->kind != MemoryRegionKind::MemoryMapped) {
    return referee::Result<IoRegionHandle>::err(referee::ErrorCode::FailedPrecondition,
                                                "memory region is not memory-mapped IO");
  }
  return referee::Result<IoRegionHandle>::ok({resource_id, access_mode, capability_context_id});
}

referee::Result<service::CapabilityContext> MachineHandleFactory::load_context(
    referee::ObjectID capability_context_id) {
  auto context = contexts_.get_context(capability_context_id);
  if (!context) return referee::Result<service::CapabilityContext>::err(context.error.value());
  if (!context.value->has_value()) {
    return referee::Result<service::CapabilityContext>::err(
        referee::ErrorCode::NotFound, "capability context not found");
  }
  return referee::Result<service::CapabilityContext>::ok(
      std::move(context.value->value().context));
}

referee::Result<MachineLease> MachineLeaseRegistry::create_lease(
    referee::ObjectID lease_id,
    referee::ObjectID owner_id,
    MachineHandle handle) {
  if (find(lease_id) != nullptr) {
    return referee::Result<MachineLease>::err(referee::ErrorCode::AlreadyExists,
                                              "duplicate Machine lease ID");
  }
  const referee::ObjectID capability_context_id = handle_context_id(handle);
  auto context_record = contexts_.get_context(capability_context_id);
  if (!context_record) {
    return referee::Result<MachineLease>::err(context_record.error.value());
  }
  if (!context_record.value->has_value()) {
    return referee::Result<MachineLease>::err(referee::ErrorCode::NotFound,
                                              "capability context not found");
  }
  const service::CapabilityContext& context = context_record.value->value().context;
  if (owner_id != context.subject) {
    return referee::Result<MachineLease>::err(referee::ErrorCode::FailedPrecondition,
                                              "lease owner does not match capability subject");
  }
  auto authorization = authorize_handle(context, handle_resource_kind(handle),
                                        handle_access_mode(handle), handle_resource_id(handle));
  if (!authorization) return referee::Result<MachineLease>::err(authorization.error.value());

  MachineLease lease(lease_id, owner_id, capability_context_id, std::move(handle));
  leases_.push_back(lease);
  return referee::Result<MachineLease>::ok(std::move(lease));
}

const MachineLease* MachineLeaseRegistry::find(referee::ObjectID lease_id) const {
  auto lease = std::find_if(leases_.begin(), leases_.end(), [&lease_id](const MachineLease& item) {
    return item.lease_id() == lease_id;
  });
  return lease == leases_.end() ? nullptr : &*lease;
}

MachineLease* MachineLeaseRegistry::find_mutable(referee::ObjectID lease_id) {
  auto lease = std::find_if(leases_.begin(), leases_.end(), [&lease_id](const MachineLease& item) {
    return item.lease_id() == lease_id;
  });
  return lease == leases_.end() ? nullptr : &*lease;
}

referee::Result<void> MachineLeaseRegistry::authorize_use(
    referee::ObjectID lease_id,
    referee::ObjectID owner_id,
    referee::ObjectID capability_context_id) const {
  const MachineLease* lease = find(lease_id);
  if (lease == nullptr) {
    return referee::Result<void>::err(referee::ErrorCode::NotFound,
                                      "Machine lease not found");
  }
  if (lease->state() != MachineLeaseState::Active) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "Machine lease is not active");
  }
  if (lease->owner_id() != owner_id) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "Machine lease owner mismatch");
  }
  if (lease->capability_context_id() != capability_context_id) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "Machine lease capability context mismatch");
  }
  return referee::Result<void>::ok();
}

referee::Result<void> MachineLeaseRegistry::release(referee::ObjectID lease_id,
                                                    referee::ObjectID owner_id) {
  MachineLease* lease = find_mutable(lease_id);
  if (lease == nullptr) {
    return referee::Result<void>::err(referee::ErrorCode::NotFound,
                                      "Machine lease not found");
  }
  if (lease->owner_id() != owner_id) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "Machine lease owner mismatch");
  }
  if (lease->state_ != MachineLeaseState::Active) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "Machine lease is not active");
  }
  lease->state_ = MachineLeaseState::Released;
  return referee::Result<void>::ok();
}

referee::Result<void> MachineLeaseRegistry::revoke(referee::ObjectID lease_id) {
  MachineLease* lease = find_mutable(lease_id);
  if (lease == nullptr) {
    return referee::Result<void>::err(referee::ErrorCode::NotFound,
                                      "Machine lease not found");
  }
  if (lease->state_ != MachineLeaseState::Active) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "Machine lease is not active");
  }
  lease->state_ = MachineLeaseState::Revoked;
  return referee::Result<void>::ok();
}

std::size_t MachineLeaseRegistry::revoke_for_owner(referee::ObjectID owner_id) {
  std::size_t revoked = 0;
  for (auto& lease : leases_) {
    if (lease.owner_id() == owner_id && lease.state_ == MachineLeaseState::Active) {
      lease.state_ = MachineLeaseState::Revoked;
      ++revoked;
    }
  }
  return revoked;
}

std::size_t MachineLeaseRegistry::revoke_for_context(referee::ObjectID capability_context_id) {
  std::size_t revoked = 0;
  for (auto& lease : leases_) {
    if (lease.capability_context_id() == capability_context_id &&
        lease.state_ == MachineLeaseState::Active) {
      lease.state_ = MachineLeaseState::Revoked;
      ++revoked;
    }
  }
  return revoked;
}

} // namespace iris::machine
