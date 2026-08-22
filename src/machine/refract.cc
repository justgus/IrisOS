#include "machine/refract.h"

#include "machine/buffers.h"
#include "machine/definitions.h"
#include "machine/inventory.h"
#include "machine/scalars.h"
#include "machine/topology.h"

#include <optional>
#include <string>
#include <utility>

namespace iris::machine {

namespace {

constexpr referee::TypeID kRefractU64Type{0x1002ULL};
constexpr referee::TypeID kRefractBoolType{0x1003ULL};
constexpr referee::TypeID kRefractObjectIDType{0x1004ULL};
constexpr referee::TypeID kRefractBytesType{0x1007ULL};
constexpr referee::TypeID kRefractStringType{0x1001ULL};

refract::DocumentationMetadata documentation(std::string summary) {
  refract::DocumentationMetadata out{};
  out.summary = std::move(summary);
  return out;
}

refract::TypeDefinition scalar(referee::TypeID type_id,
                               std::string name,
                               referee::TypeID value_type,
                               std::string summary) {
  refract::TypeDefinition def{};
  def.type_id = type_id;
  def.name = std::move(name);
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "primitive";
  def.fields.push_back(refract::FieldDefinition{"value", value_type, true, std::nullopt});
  def.documentation = documentation(std::move(summary));
  return def;
}

refract::TypeDefinition byte_order_definition() {
  refract::TypeDefinition def{};
  def.type_id = kByteOrderType;
  def.name = "ByteOrder";
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "enum";
  def.has_enum_value_type = true;
  def.enum_value_type = kRefractU64Type;
  def.enum_values = {
      {"Little", "0"},
      {"Big", "1"},
      {"Native", "2"}
  };
  def.documentation = documentation("Identifies byte ordering without converting values.");
  return def;
}

refract::TypeDefinition address_definition() {
  refract::TypeDefinition def{};
  def.type_id = kAddressType;
  def.name = "Address";
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "struct";
  def.fields.push_back(refract::FieldDefinition{"value", kUInt128Type, true, std::nullopt});
  def.fields.push_back(refract::FieldDefinition{"bit_width", kRefractU64Type, true, std::nullopt});
  def.documentation = documentation("A pointer-free address value with an explicit width.");
  return def;
}

refract::TypeDefinition blob_definition() {
  auto def = scalar(kBlobType, "Blob", kRefractBytesType,
                    "An immutable owning sequence of Machine bytes.");
  def.kind = "collection";
  def.collection_kind = "sequence";
  def.collection_elements.push_back(refract::CollectionElementDefinition{"element", kByteType});
  return def;
}

refract::TypeDefinition span_definition() {
  refract::TypeDefinition def{};
  def.type_id = kSpanType;
  def.name = "Span";
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "struct";
  def.fields.push_back(refract::FieldDefinition{"offset", kRefractU64Type, true, std::nullopt});
  def.fields.push_back(refract::FieldDefinition{"length", kRefractU64Type, true, std::nullopt});
  def.documentation = documentation("A pointer-free offset and length range.");
  return def;
}

refract::TypeDefinition slice_definition() {
  refract::TypeDefinition def{};
  def.type_id = kSliceType;
  def.name = "Slice";
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "struct";
  def.fields.push_back(refract::FieldDefinition{"blob", kBlobType, true, std::nullopt});
  def.fields.push_back(refract::FieldDefinition{"span", kSpanType, true, std::nullopt});
  def.documentation = documentation("An immutable, shared-ownership view into a Machine blob.");
  return def;
}

refract::TypeDefinition packet_definition() {
  refract::TypeDefinition def{};
  def.type_id = kPacketType;
  def.name = "Packet";
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "struct";
  def.fields.push_back(refract::FieldDefinition{"payload", kBlobType, true, std::nullopt});
  def.documentation = documentation("An immutable Machine packet payload without transport policy.");
  return def;
}

refract::TypeDefinition enumeration(referee::TypeID type_id,
                                    std::string name,
                                    std::vector<refract::EnumValueDefinition> values,
                                    std::string summary) {
  refract::TypeDefinition def{};
  def.type_id = type_id;
  def.name = std::move(name);
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "enum";
  def.has_enum_value_type = true;
  def.enum_value_type = kRefractU64Type;
  def.enum_values = std::move(values);
  def.documentation = documentation(std::move(summary));
  return def;
}

refract::TypeDefinition structure(
    referee::TypeID type_id,
    std::string name,
    std::vector<refract::FieldDefinition> fields,
    std::string summary) {
  refract::TypeDefinition def{};
  def.type_id = type_id;
  def.name = std::move(name);
  def.namespace_name = "Machine";
  def.version = 1;
  def.kind = "struct";
  def.fields = std::move(fields);
  def.documentation = documentation(std::move(summary));
  return def;
}

} // namespace

std::vector<refract::TypeDefinition> schema_definitions() {
  std::vector<refract::TypeDefinition> defs;
  defs.reserve(27);
  defs.push_back(scalar(kBitType, "Bit", kRefractU64Type, "A validated one-bit value."));
  defs.push_back(scalar(kNibbleType, "Nibble", kRefractU64Type, "A validated four-bit value."));
  defs.push_back(scalar(kByteType, "Byte", kRefractU64Type, "An unsigned eight-bit value."));
  defs.push_back(scalar(kWord16Type, "Word16", kRefractU64Type, "An unsigned sixteen-bit word."));
  defs.push_back(scalar(kWord32Type, "Word32", kRefractU64Type, "An unsigned thirty-two-bit word."));
  defs.push_back(scalar(kWord64Type, "Word64", kRefractU64Type, "An unsigned sixty-four-bit word."));
  defs.push_back(scalar(kUInt128Type, "UInt128", kRefractBytesType, "A portable unsigned 128-bit value."));
  defs.push_back(byte_order_definition());
  defs.push_back(scalar(kAlignmentType, "Alignment", kRefractU64Type, "A nonzero power-of-two alignment."));
  defs.push_back(address_definition());
  defs.push_back(blob_definition());
  defs.push_back(span_definition());
  defs.push_back(slice_definition());
  defs.push_back(packet_definition());
  defs.push_back(enumeration(kRegisterRoleType, "RegisterRole", {
      {"GeneralPurpose", "0"}, {"ProgramCounter", "1"}, {"StackPointer", "2"},
      {"Status", "3"}, {"Control", "4"}, {"FloatingPoint", "5"},
      {"Vector", "6"}, {"Other", "7"}}, "Describes a register's architectural role."));
  defs.push_back(structure(kRegisterDefinitionType, "RegisterDefinition", {
      {"definition_id", kRefractObjectIDType, true, std::nullopt},
      {"name", kRefractStringType, true, std::nullopt},
      {"bit_width", kRefractU64Type, true, std::nullopt},
      {"role", kRegisterRoleType, true, std::nullopt}},
      "Defines register layout without storing a live value."));
  defs.push_back(structure(kCoreDefinitionType, "CoreDefinition", {
      {"definition_id", kRefractObjectIDType, true, std::nullopt},
      {"name", kRefractStringType, true, std::nullopt}},
      "Defines a processor core kind without live machine state."));
  defs.push_back(structure(kArchitectureDefinitionType, "ArchitectureDefinition", {
      {"definition_id", kRefractObjectIDType, true, std::nullopt},
      {"name", kRefractStringType, true, std::nullopt},
      {"byte_order", kByteOrderType, true, std::nullopt},
      {"address_width", kRefractU64Type, true, std::nullopt},
      {"core_definitions", kRefractBytesType, true, std::nullopt},
      {"register_definitions", kRefractBytesType, true, std::nullopt}},
      "An ordered, stable processor architecture definition."));
  defs.push_back(enumeration(kMemoryRegionKindType, "MemoryRegionKind", {
      {"Ram", "0"}, {"Rom", "1"}, {"Firmware", "2"},
      {"MemoryMapped", "3"}, {"Reserved", "4"}, {"Other", "5"}},
      "Classifies a descriptive memory region."));
  defs.push_back(structure(kMemoryRegionType, "MemoryRegion", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"start", kAddressType, true, std::nullopt},
      {"size", kUInt128Type, true, std::nullopt},
      {"alignment", kAlignmentType, false, std::nullopt},
      {"kind", kMemoryRegionKindType, true, std::nullopt},
      {"name", kRefractStringType, true, std::nullopt}},
      "A descriptive memory range that grants no access authority."));
  defs.push_back(structure(kAvailableMemoryBlockType, "AvailableMemoryBlock", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"region_id", kRefractObjectIDType, true, std::nullopt},
      {"start", kAddressType, true, std::nullopt},
      {"size", kUInt128Type, true, std::nullopt},
      {"alignment", kAlignmentType, false, std::nullopt}},
      "A contained availability fact that grants no allocation authority."));
  defs.push_back(structure(kAddressSpaceType, "AddressSpace", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"address_width", kRefractU64Type, true, std::nullopt},
      {"regions", kRefractBytesType, true, std::nullopt},
      {"available_blocks", kRefractBytesType, true, std::nullopt}},
      "An ordered collection of descriptive memory topology."));
  defs.push_back(structure(kCoreDescriptorType, "CoreDescriptor", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"architecture_definition_id", kRefractObjectIDType, true, std::nullopt},
      {"core_definition_id", kRefractObjectIDType, true, std::nullopt},
      {"logical_index", kRefractU64Type, true, std::nullopt},
      {"enabled", kRefractBoolType, true, std::nullopt}},
      "A concrete processor core fact without operations or authority."));
  defs.push_back(structure(kRegisterFileDescriptorType, "RegisterFileDescriptor", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"core_id", kRefractObjectIDType, true, std::nullopt},
      {"register_definition_ids", kRefractBytesType, true, std::nullopt}},
      "Identifies registers present on a core without storing their values."));
  defs.push_back(structure(kBusDescriptorType, "BusDescriptor", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"type", kRefractStringType, true, std::nullopt},
      {"name", kRefractStringType, true, std::nullopt},
      {"parent_bus_id", kRefractObjectIDType, false, std::nullopt}},
      "A descriptive bus fact without driver handles or operations."));
  defs.push_back(structure(kDeviceDescriptorType, "DeviceDescriptor", {
      {"resource_id", kRefractObjectIDType, true, std::nullopt},
      {"type", kRefractStringType, true, std::nullopt},
      {"name", kRefractStringType, true, std::nullopt},
      {"parent_bus_id", kRefractObjectIDType, true, std::nullopt}},
      "A descriptive device fact without driver handles or operations."));
  defs.push_back(structure(kMachineInventoryType, "MachineInventory", {
      {"architectures", kRefractBytesType, true, std::nullopt},
      {"cores", kRefractBytesType, true, std::nullopt},
      {"register_files", kRefractBytesType, true, std::nullopt},
      {"address_spaces", kRefractBytesType, true, std::nullopt},
      {"buses", kRefractBytesType, true, std::nullopt},
      {"devices", kRefractBytesType, true, std::nullopt}},
      "An immutable, deterministically ordered in-memory machine inventory."));
  return defs;
}

} // namespace iris::machine
