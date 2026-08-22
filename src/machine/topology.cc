#include "machine/topology.h"

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

bool less(const UInt128& left, const UInt128& right) {
  return left.bytes() < right.bytes();
}

bool is_zero(const UInt128& value) {
  return value == UInt128{};
}

std::optional<UInt128> add(const UInt128& left, const UInt128& right) {
  UInt128::Bytes bytes{};
  unsigned carry = 0;
  for (std::size_t i = bytes.size(); i-- > 0;) {
    const unsigned sum = static_cast<unsigned>(left.bytes()[i])
        + static_cast<unsigned>(right.bytes()[i]) + carry;
    bytes[i] = static_cast<std::uint8_t>(sum & 0xFFU);
    carry = sum >> 8U;
  }
  if (carry != 0) return std::nullopt;
  return UInt128(bytes);
}

bool aligned(const UInt128& value, Alignment alignment) {
  std::uint64_t low_bits = 0;
  for (const auto byte : value.bytes()) {
    low_bits = (low_bits << 8U) | byte;
  }
  return (low_bits & (alignment.bytes() - 1U)) == 0;
}

bool within_address_space(const UInt128& end, std::uint8_t width) {
  if (width == 128) return true;
  UInt128::Bytes limit_bytes{};
  const auto byte_from_end = static_cast<std::size_t>(width / 8U);
  const auto bit_in_byte = static_cast<std::uint8_t>(width % 8U);
  limit_bytes[15U - byte_from_end] = static_cast<std::uint8_t>(1U << bit_in_byte);
  return !less(UInt128(limit_bytes), end);
}

bool valid_alignment(const Address& start,
                     const UInt128& size,
                     const std::optional<Alignment>& alignment) {
  return !alignment.has_value()
      || (aligned(start.value(), *alignment) && aligned(size, *alignment));
}

bool ranges_overlap(const Address& left_start,
                    const UInt128& left_size,
                    const Address& right_start,
                    const UInt128& right_size) {
  if (is_zero(left_size) || is_zero(right_size)) return false;
  const auto left_end = add(left_start.value(), left_size);
  const auto right_end = add(right_start.value(), right_size);
  return left_end.has_value() && right_end.has_value()
      && less(left_start.value(), *right_end)
      && less(right_start.value(), *left_end);
}

} // namespace

AddressSpace::AddressSpace(referee::ObjectID resource_id,
                           std::uint8_t address_width,
                           std::vector<MemoryRegion> regions,
                           std::vector<AvailableMemoryBlock> available_blocks)
    : resource_id_(resource_id),
      address_width_(address_width),
      regions_(std::move(regions)),
      available_blocks_(std::move(available_blocks)) {}

referee::Result<AddressSpace> AddressSpace::create(
    referee::ObjectID resource_id,
    std::uint8_t address_width,
    std::vector<MemoryRegion> regions,
    std::vector<AvailableMemoryBlock> available_blocks) {
  if (empty_id(resource_id) || address_width == 0 || address_width > 128) {
    return referee::Result<AddressSpace>::err(
        referee::ErrorCode::InvalidArgument,
        "address space ID must be nonempty and width must be between 1 and 128 bits");
  }

  std::set<referee::ObjectID, ObjectIDLess> ids;
  for (const auto& region : regions) {
    if (empty_id(region.resource_id) || region.start.bit_width() != address_width) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "memory region ID and address width must match its address space");
    }
    if (!ids.insert(region.resource_id).second) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::AlreadyExists, "duplicate memory resource ID");
    }
    if (!valid_alignment(region.start, region.size, region.alignment)) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "memory region start and size must satisfy its alignment");
    }
    const auto region_end = add(region.start.value(), region.size);
    if (!region_end.has_value() || !within_address_space(*region_end, address_width)) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "memory region range exceeds its address space");
    }
  }

  for (const auto& block : available_blocks) {
    if (empty_id(block.resource_id) || block.start.bit_width() != address_width) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "available block ID and address width must match its address space");
    }
    if (!ids.insert(block.resource_id).second) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::AlreadyExists, "duplicate memory resource ID");
    }
    if (!valid_alignment(block.start, block.size, block.alignment)) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "available block start and size must satisfy its alignment");
    }
    const auto block_end = add(block.start.value(), block.size);
    if (!block_end.has_value() || !within_address_space(*block_end, address_width)) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "available block range exceeds its address space");
    }
    const auto parent = std::find_if(regions.begin(), regions.end(), [&](const auto& region) {
      return region.resource_id == block.region_id;
    });
    if (parent == regions.end()) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::NotFound, "available block references an unknown region");
    }
    const auto parent_end = add(parent->start.value(), parent->size);
    const bool begins_inside = !less(block.start.value(), parent->start.value());
    const bool ends_inside = parent_end.has_value() && !less(*parent_end, *block_end);
    if (!begins_inside || !ends_inside) {
      return referee::Result<AddressSpace>::err(
          referee::ErrorCode::InvalidArgument,
          "available block must be contained within its parent region");
    }
  }

  for (std::size_t i = 0; i < available_blocks.size(); ++i) {
    for (std::size_t j = i + 1; j < available_blocks.size(); ++j) {
      if (ranges_overlap(available_blocks[i].start, available_blocks[i].size,
                         available_blocks[j].start, available_blocks[j].size)) {
        return referee::Result<AddressSpace>::err(
            referee::ErrorCode::InvalidArgument,
            "available memory blocks must not overlap");
      }
    }
  }

  return referee::Result<AddressSpace>::ok(AddressSpace(
      resource_id, address_width, std::move(regions), std::move(available_blocks)));
}

} // namespace iris::machine
