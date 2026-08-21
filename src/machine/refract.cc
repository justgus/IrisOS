#include "machine/refract.h"

#include "machine/buffers.h"
#include "machine/scalars.h"

#include <optional>
#include <string>
#include <utility>

namespace iris::machine {

namespace {

constexpr referee::TypeID kRefractU64Type{0x1002ULL};
constexpr referee::TypeID kRefractBytesType{0x1007ULL};

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

} // namespace

std::vector<refract::TypeDefinition> schema_definitions() {
  std::vector<refract::TypeDefinition> defs;
  defs.reserve(14);
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
  return defs;
}

} // namespace iris::machine
