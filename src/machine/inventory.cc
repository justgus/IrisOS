#include "machine/inventory.h"

#include <algorithm>
#include <set>
#include <utility>

namespace iris::machine {

namespace {

struct ObjectIDLess {
  bool operator()(const referee::ObjectID& left, const referee::ObjectID& right) const {
    return left.bytes < right.bytes;
  }
};

bool empty_id(const referee::ObjectID& id) {
  return id == referee::ObjectID{};
}

template <typename T, typename ID>
const T* find_by_id(const std::vector<T>& values, referee::ObjectID id, ID get_id) {
  const auto found = std::lower_bound(values.begin(), values.end(), id,
      [&](const T& value, const referee::ObjectID& wanted) {
        return get_id(value).bytes < wanted.bytes;
      });
  return found != values.end() && get_id(*found) == id ? &*found : nullptr;
}

template <typename T, typename ID>
void sort_by_id(std::vector<T>& values, ID get_id) {
  std::sort(values.begin(), values.end(), [&](const T& left, const T& right) {
    return get_id(left).bytes < get_id(right).bytes;
  });
}

} // namespace

MachineInventory::MachineInventory(
    std::vector<ArchitectureDefinition> architectures,
    std::vector<CoreDescriptor> cores,
    std::vector<RegisterFileDescriptor> register_files,
    std::vector<AddressSpace> address_spaces,
    std::vector<BusDescriptor> buses,
    std::vector<DeviceDescriptor> devices)
    : architectures_(std::move(architectures)),
      cores_(std::move(cores)),
      register_files_(std::move(register_files)),
      address_spaces_(std::move(address_spaces)),
      buses_(std::move(buses)),
      devices_(std::move(devices)) {}

referee::Result<MachineInventory> MachineInventory::create(
    std::vector<ArchitectureDefinition> architectures,
    std::vector<CoreDescriptor> cores,
    std::vector<RegisterFileDescriptor> register_files,
    std::vector<AddressSpace> address_spaces,
    std::vector<BusDescriptor> buses,
    std::vector<DeviceDescriptor> devices) {
  sort_by_id(architectures, [](const auto& value) { return value.definition_id(); });
  sort_by_id(cores, [](const auto& value) { return value.resource_id; });
  sort_by_id(register_files, [](const auto& value) { return value.resource_id; });
  sort_by_id(address_spaces, [](const auto& value) { return value.resource_id(); });
  sort_by_id(buses, [](const auto& value) { return value.resource_id; });
  sort_by_id(devices, [](const auto& value) { return value.resource_id; });

  std::set<referee::ObjectID, ObjectIDLess> definition_ids;
  for (const auto& architecture : architectures) {
    if (!definition_ids.insert(architecture.definition_id()).second) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::AlreadyExists, "duplicate definition ID");
    }
    for (const auto& core : architecture.core_definitions()) {
      if (!definition_ids.insert(core.definition_id).second) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::AlreadyExists, "duplicate definition ID");
      }
    }
    for (const auto& reg : architecture.register_definitions()) {
      if (!definition_ids.insert(reg.definition_id).second) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::AlreadyExists, "duplicate definition ID");
      }
    }
  }

  std::set<referee::ObjectID, ObjectIDLess> resource_ids;
  const auto add_resource = [&](const referee::ObjectID& id) {
    return !empty_id(id) && resource_ids.insert(id).second;
  };
  for (const auto& core : cores) {
    if (!add_resource(core.resource_id)) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
    }
  }
  for (const auto& file : register_files) {
    if (!add_resource(file.resource_id)) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
    }
  }
  for (const auto& space : address_spaces) {
    if (!add_resource(space.resource_id())) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
    }
    for (const auto& region : space.regions()) {
      if (!add_resource(region.resource_id)) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
      }
    }
    for (const auto& block : space.available_blocks()) {
      if (!add_resource(block.resource_id)) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
      }
    }
  }
  for (const auto& bus : buses) {
    if (!add_resource(bus.resource_id)) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
    }
  }
  for (const auto& device : devices) {
    if (!add_resource(device.resource_id)) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::AlreadyExists, "empty or duplicate resource ID");
    }
  }

  MachineInventory inventory(std::move(architectures), std::move(cores),
      std::move(register_files), std::move(address_spaces), std::move(buses),
      std::move(devices));

  for (const auto& core : inventory.cores_) {
    const auto* architecture = inventory.find_architecture(core.architecture_definition_id);
    if (architecture == nullptr) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::NotFound, "core references an unknown architecture definition");
    }
    const auto found = std::find_if(architecture->core_definitions().begin(),
        architecture->core_definitions().end(), [&](const auto& definition) {
          return definition.definition_id == core.core_definition_id;
        });
    if (found == architecture->core_definitions().end()) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::NotFound, "core references an unknown core definition");
    }
  }

  for (const auto& file : inventory.register_files_) {
    const auto* core = inventory.find_core(file.core_id);
    if (core == nullptr) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::NotFound, "register file references an unknown core");
    }
    const auto* architecture = inventory.find_architecture(core->architecture_definition_id);
    std::set<referee::ObjectID, ObjectIDLess> seen;
    for (const auto& id : file.register_definition_ids) {
      const auto found = std::find_if(architecture->register_definitions().begin(),
          architecture->register_definitions().end(), [&](const auto& definition) {
            return definition.definition_id == id;
          });
      if (found == architecture->register_definitions().end()) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::NotFound,
            "register file references an unknown register definition");
      }
      if (!seen.insert(id).second) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::AlreadyExists,
            "register file contains a duplicate register definition");
      }
    }
  }

  for (const auto& bus : inventory.buses_) {
    if (bus.type.empty() || bus.name.empty()) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::InvalidArgument, "bus type and name must be nonempty");
    }
    if (bus.parent_bus_id.has_value()
        && inventory.find_bus(*bus.parent_bus_id) == nullptr) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::NotFound, "bus references an unknown parent bus");
    }
    std::set<referee::ObjectID, ObjectIDLess> ancestors;
    const BusDescriptor* current = &bus;
    while (current->parent_bus_id.has_value()) {
      if (!ancestors.insert(current->resource_id).second) {
        return referee::Result<MachineInventory>::err(
            referee::ErrorCode::InvalidArgument, "bus parent relationship contains a cycle");
      }
      current = inventory.find_bus(*current->parent_bus_id);
    }
  }
  for (const auto& device : inventory.devices_) {
    if (device.type.empty() || device.name.empty()) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::InvalidArgument, "device type and name must be nonempty");
    }
    if (inventory.find_bus(device.parent_bus_id) == nullptr) {
      return referee::Result<MachineInventory>::err(
          referee::ErrorCode::NotFound, "device references an unknown parent bus");
    }
  }

  return referee::Result<MachineInventory>::ok(std::move(inventory));
}

const ArchitectureDefinition* MachineInventory::find_architecture(referee::ObjectID id) const {
  return find_by_id(architectures_, id, [](const auto& value) { return value.definition_id(); });
}

const CoreDescriptor* MachineInventory::find_core(referee::ObjectID id) const {
  return find_by_id(cores_, id, [](const auto& value) { return value.resource_id; });
}

const RegisterFileDescriptor* MachineInventory::find_register_file(referee::ObjectID id) const {
  return find_by_id(register_files_, id, [](const auto& value) { return value.resource_id; });
}

const AddressSpace* MachineInventory::find_address_space(referee::ObjectID id) const {
  return find_by_id(address_spaces_, id, [](const auto& value) { return value.resource_id(); });
}

const MemoryRegion* MachineInventory::find_memory_region(referee::ObjectID id) const {
  for (const auto& space : address_spaces_) {
    const auto found = std::find_if(space.regions().begin(), space.regions().end(),
        [&](const auto& region) { return region.resource_id == id; });
    if (found != space.regions().end()) return &*found;
  }
  return nullptr;
}

const AvailableMemoryBlock* MachineInventory::find_available_memory_block(
    referee::ObjectID id) const {
  for (const auto& space : address_spaces_) {
    const auto found = std::find_if(space.available_blocks().begin(),
        space.available_blocks().end(),
        [&](const auto& block) { return block.resource_id == id; });
    if (found != space.available_blocks().end()) return &*found;
  }
  return nullptr;
}

std::vector<std::reference_wrapper<const MemoryRegion>> MachineInventory::memory_regions() const {
  std::vector<std::reference_wrapper<const MemoryRegion>> result;
  for (const auto& space : address_spaces_) {
    for (const auto& region : space.regions()) result.emplace_back(region);
  }
  std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
    return left.get().resource_id.bytes < right.get().resource_id.bytes;
  });
  return result;
}

std::vector<std::reference_wrapper<const AvailableMemoryBlock>>
MachineInventory::available_memory_blocks() const {
  std::vector<std::reference_wrapper<const AvailableMemoryBlock>> result;
  for (const auto& space : address_spaces_) {
    for (const auto& block : space.available_blocks()) result.emplace_back(block);
  }
  std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
    return left.get().resource_id.bytes < right.get().resource_id.bytes;
  });
  return result;
}

const BusDescriptor* MachineInventory::find_bus(referee::ObjectID id) const {
  return find_by_id(buses_, id, [](const auto& value) { return value.resource_id; });
}

const DeviceDescriptor* MachineInventory::find_device(referee::ObjectID id) const {
  return find_by_id(devices_, id, [](const auto& value) { return value.resource_id; });
}

} // namespace iris::machine
