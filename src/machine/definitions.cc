#include "machine/definitions.h"

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

} // namespace

ArchitectureDefinition::ArchitectureDefinition(
    referee::ObjectID definition_id,
    std::string name,
    ByteOrder byte_order,
    std::uint8_t address_width,
    std::vector<CoreDefinition> core_definitions,
    std::vector<RegisterDefinition> register_definitions)
    : definition_id_(definition_id),
      name_(std::move(name)),
      byte_order_(byte_order),
      address_width_(address_width),
      core_definitions_(std::move(core_definitions)),
      register_definitions_(std::move(register_definitions)) {}

referee::Result<ArchitectureDefinition> ArchitectureDefinition::create(
    referee::ObjectID definition_id,
    std::string name,
    ByteOrder byte_order,
    std::uint8_t address_width,
    std::vector<CoreDefinition> core_definitions,
    std::vector<RegisterDefinition> register_definitions) {
  if (empty_id(definition_id) || name.empty()) {
    return referee::Result<ArchitectureDefinition>::err(
        referee::ErrorCode::InvalidArgument,
        "architecture definition ID and name must be nonempty");
  }
  if (address_width == 0 || address_width > 128) {
    return referee::Result<ArchitectureDefinition>::err(
        referee::ErrorCode::InvalidArgument,
        "architecture address width must be between 1 and 128 bits");
  }

  std::set<referee::ObjectID, ObjectIDLess> ids;
  for (const auto& core : core_definitions) {
    if (empty_id(core.definition_id) || core.name.empty()) {
      return referee::Result<ArchitectureDefinition>::err(
          referee::ErrorCode::InvalidArgument,
          "core definition ID and name must be nonempty");
    }
    if (!ids.insert(core.definition_id).second) {
      return referee::Result<ArchitectureDefinition>::err(
          referee::ErrorCode::AlreadyExists, "duplicate definition ID");
    }
  }
  for (const auto& reg : register_definitions) {
    if (empty_id(reg.definition_id) || reg.name.empty() || reg.bit_width == 0) {
      return referee::Result<ArchitectureDefinition>::err(
          referee::ErrorCode::InvalidArgument,
          "register definition ID, name, and bit width must be nonempty");
    }
    if (!ids.insert(reg.definition_id).second) {
      return referee::Result<ArchitectureDefinition>::err(
          referee::ErrorCode::AlreadyExists, "duplicate definition ID");
    }
  }

  return referee::Result<ArchitectureDefinition>::ok(ArchitectureDefinition(
      definition_id, std::move(name), byte_order, address_width,
      std::move(core_definitions), std::move(register_definitions)));
}

} // namespace iris::machine
