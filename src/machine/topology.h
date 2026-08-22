#pragma once

#include "machine/scalars.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace iris::machine {

constexpr referee::TypeID kMemoryRegionKindType{0x4D41434800000013ULL};
constexpr referee::TypeID kMemoryRegionType{0x4D41434800000014ULL};
constexpr referee::TypeID kAvailableMemoryBlockType{0x4D41434800000015ULL};
constexpr referee::TypeID kAddressSpaceType{0x4D41434800000016ULL};

enum class MemoryRegionKind {
  Ram,
  Rom,
  Firmware,
  MemoryMapped,
  Reserved,
  Other
};

struct MemoryRegion {
  referee::ObjectID resource_id{};
  Address start;
  UInt128 size;
  std::optional<Alignment> alignment;
  MemoryRegionKind kind{MemoryRegionKind::Other};
  std::string name;
};

struct AvailableMemoryBlock {
  referee::ObjectID resource_id{};
  referee::ObjectID region_id{};
  Address start;
  UInt128 size;
  std::optional<Alignment> alignment;
};

class AddressSpace {
public:
  static referee::Result<AddressSpace> create(
      referee::ObjectID resource_id,
      std::uint8_t address_width,
      std::vector<MemoryRegion> regions,
      std::vector<AvailableMemoryBlock> available_blocks);

  const referee::ObjectID& resource_id() const { return resource_id_; }
  std::uint8_t address_width() const { return address_width_; }
  const std::vector<MemoryRegion>& regions() const { return regions_; }
  const std::vector<AvailableMemoryBlock>& available_blocks() const {
    return available_blocks_;
  }

private:
  AddressSpace(referee::ObjectID resource_id,
               std::uint8_t address_width,
               std::vector<MemoryRegion> regions,
               std::vector<AvailableMemoryBlock> available_blocks);

  referee::ObjectID resource_id_{};
  std::uint8_t address_width_{};
  std::vector<MemoryRegion> regions_;
  std::vector<AvailableMemoryBlock> available_blocks_;
};

} // namespace iris::machine
