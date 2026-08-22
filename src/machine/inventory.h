#pragma once

#include "machine/definitions.h"
#include "machine/topology.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace iris::machine {

constexpr referee::TypeID kCoreDescriptorType{0x4D41434800000017ULL};
constexpr referee::TypeID kRegisterFileDescriptorType{0x4D41434800000018ULL};
constexpr referee::TypeID kBusDescriptorType{0x4D41434800000019ULL};
constexpr referee::TypeID kDeviceDescriptorType{0x4D4143480000001AULL};
constexpr referee::TypeID kMachineInventoryType{0x4D4143480000001BULL};

struct CoreDescriptor {
  referee::ObjectID resource_id{};
  referee::ObjectID architecture_definition_id{};
  referee::ObjectID core_definition_id{};
  std::uint32_t logical_index{};
  bool enabled{};
};

struct RegisterFileDescriptor {
  referee::ObjectID resource_id{};
  referee::ObjectID core_id{};
  std::vector<referee::ObjectID> register_definition_ids;
};

struct BusDescriptor {
  referee::ObjectID resource_id{};
  std::string type;
  std::string name;
  std::optional<referee::ObjectID> parent_bus_id;
};

struct DeviceDescriptor {
  referee::ObjectID resource_id{};
  std::string type;
  std::string name;
  referee::ObjectID parent_bus_id{};
};

class MachineInventory {
public:
  static referee::Result<MachineInventory> create(
      std::vector<ArchitectureDefinition> architectures,
      std::vector<CoreDescriptor> cores,
      std::vector<RegisterFileDescriptor> register_files,
      std::vector<AddressSpace> address_spaces,
      std::vector<BusDescriptor> buses,
      std::vector<DeviceDescriptor> devices);

  const ArchitectureDefinition* find_architecture(referee::ObjectID id) const;
  const CoreDescriptor* find_core(referee::ObjectID id) const;
  const RegisterFileDescriptor* find_register_file(referee::ObjectID id) const;
  const AddressSpace* find_address_space(referee::ObjectID id) const;
  const MemoryRegion* find_memory_region(referee::ObjectID id) const;
  const AvailableMemoryBlock* find_available_memory_block(referee::ObjectID id) const;
  const BusDescriptor* find_bus(referee::ObjectID id) const;
  const DeviceDescriptor* find_device(referee::ObjectID id) const;

  const std::vector<ArchitectureDefinition>& architectures() const { return architectures_; }
  const std::vector<CoreDescriptor>& cores() const { return cores_; }
  const std::vector<RegisterFileDescriptor>& register_files() const { return register_files_; }
  const std::vector<AddressSpace>& address_spaces() const { return address_spaces_; }
  std::vector<std::reference_wrapper<const MemoryRegion>> memory_regions() const;
  std::vector<std::reference_wrapper<const AvailableMemoryBlock>> available_memory_blocks() const;
  const std::vector<BusDescriptor>& buses() const { return buses_; }
  const std::vector<DeviceDescriptor>& devices() const { return devices_; }

private:
  MachineInventory(std::vector<ArchitectureDefinition> architectures,
                   std::vector<CoreDescriptor> cores,
                   std::vector<RegisterFileDescriptor> register_files,
                   std::vector<AddressSpace> address_spaces,
                   std::vector<BusDescriptor> buses,
                   std::vector<DeviceDescriptor> devices);

  std::vector<ArchitectureDefinition> architectures_;
  std::vector<CoreDescriptor> cores_;
  std::vector<RegisterFileDescriptor> register_files_;
  std::vector<AddressSpace> address_spaces_;
  std::vector<BusDescriptor> buses_;
  std::vector<DeviceDescriptor> devices_;
};

} // namespace iris::machine
