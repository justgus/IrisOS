#pragma once

#include "machine/scalars.h"

#include <cstdint>
#include <string>
#include <vector>

namespace iris::machine {

constexpr referee::TypeID kRegisterRoleType{0x4D4143480000000FULL};
constexpr referee::TypeID kRegisterDefinitionType{0x4D41434800000010ULL};
constexpr referee::TypeID kCoreDefinitionType{0x4D41434800000011ULL};
constexpr referee::TypeID kArchitectureDefinitionType{0x4D41434800000012ULL};

enum class RegisterRole {
  GeneralPurpose,
  ProgramCounter,
  StackPointer,
  Status,
  Control,
  FloatingPoint,
  Vector,
  Other
};

struct RegisterDefinition {
  referee::ObjectID definition_id{};
  std::string name;
  std::uint16_t bit_width{};
  RegisterRole role{RegisterRole::Other};
};

struct CoreDefinition {
  referee::ObjectID definition_id{};
  std::string name;
};

class ArchitectureDefinition {
public:
  static referee::Result<ArchitectureDefinition> create(
      referee::ObjectID definition_id,
      std::string name,
      ByteOrder byte_order,
      std::uint8_t address_width,
      std::vector<CoreDefinition> core_definitions,
      std::vector<RegisterDefinition> register_definitions);

  const referee::ObjectID& definition_id() const { return definition_id_; }
  const std::string& name() const { return name_; }
  ByteOrder byte_order() const { return byte_order_; }
  std::uint8_t address_width() const { return address_width_; }
  const std::vector<CoreDefinition>& core_definitions() const { return core_definitions_; }
  const std::vector<RegisterDefinition>& register_definitions() const {
    return register_definitions_;
  }

private:
  ArchitectureDefinition(referee::ObjectID definition_id,
                         std::string name,
                         ByteOrder byte_order,
                         std::uint8_t address_width,
                         std::vector<CoreDefinition> core_definitions,
                         std::vector<RegisterDefinition> register_definitions);

  referee::ObjectID definition_id_{};
  std::string name_;
  ByteOrder byte_order_{ByteOrder::Native};
  std::uint8_t address_width_{};
  std::vector<CoreDefinition> core_definitions_;
  std::vector<RegisterDefinition> register_definitions_;
};

} // namespace iris::machine
