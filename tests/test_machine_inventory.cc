extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "machine/definitions.h"
#include "machine/inventory.h"
#include "machine/topology.h"
#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee_sqlite/sqlite_store.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

using namespace iris::machine;

namespace {

referee::ObjectID id(std::uint8_t value) {
  referee::ObjectID out{};
  out.bytes.back() = value;
  return out;
}

Address address(std::uint64_t value, std::uint8_t width = 64) {
  auto result = Address::create(UInt128(value), width);
  ck_assert_msg(result, "address construction failed");
  return *result.value;
}

Alignment alignment(std::uint64_t bytes) {
  auto result = Alignment::create(bytes);
  ck_assert_msg(result, "alignment construction failed");
  return *result.value;
}

ArchitectureDefinition architecture() {
  auto result = ArchitectureDefinition::create(
      id(1), "iris64", ByteOrder::Little, 64,
      {{id(2), "performance"}},
      {{id(3), "pc", 64, RegisterRole::ProgramCounter},
       {id(4), "sp", 64, RegisterRole::StackPointer}});
  ck_assert_msg(result, "architecture construction failed");
  return std::move(*result.value);
}

AddressSpace address_space(std::vector<MemoryRegion> regions,
                           std::vector<AvailableMemoryBlock> blocks) {
  auto result = AddressSpace::create(id(10), 64, std::move(regions), std::move(blocks));
  ck_assert_msg(result, "address-space construction failed: %s",
                result.error.has_value() ? result.error->message.c_str() : "ok");
  return std::move(*result.value);
}

} // namespace

START_TEST(test_architecture_definitions_preserve_order_and_state_boundary)
{
  auto arch = architecture();
  ck_assert_msg(arch.definition_id() == id(1), "architecture ID changed");
  ck_assert_str_eq(arch.name().c_str(), "iris64");
  ck_assert_uint_eq(arch.address_width(), 64U);
  ck_assert_uint_eq(arch.core_definitions().size(), 1U);
  ck_assert_uint_eq(arch.register_definitions().size(), 2U);
  ck_assert_str_eq(arch.register_definitions()[0].name.c_str(), "pc");
  ck_assert_str_eq(arch.register_definitions()[1].name.c_str(), "sp");

  auto duplicate = ArchitectureDefinition::create(
      id(1), "invalid", ByteOrder::Little, 64,
      {{id(2), "core"}}, {{id(2), "r0", 64, RegisterRole::GeneralPurpose}});
  ck_assert_msg(!duplicate, "duplicate nested definition ID accepted");
  ck_assert_int_eq(static_cast<int>(duplicate.error->code),
                   static_cast<int>(referee::ErrorCode::AlreadyExists));
}
END_TEST

START_TEST(test_topology_allows_zero_sizes_and_optional_alignment)
{
  MemoryRegion unaligned{id(11), address(3), UInt128(5), std::nullopt,
                         MemoryRegionKind::MemoryMapped, "unaligned"};
  MemoryRegion aligned{id(12), address(0x1000), UInt128(0x1000), alignment(0x1000),
                       MemoryRegionKind::Ram, "aligned"};
  AvailableMemoryBlock empty_at_end{id(13), id(12), address(0x2000), UInt128(0),
                                    alignment(0x1000)};
  auto result = AddressSpace::create(id(10), 64, {unaligned, aligned}, {empty_at_end});
  ck_assert_msg(result, "valid optional-alignment topology rejected: %s",
                result.error.has_value() ? result.error->message.c_str() : "ok");
  ck_assert_uint_eq(result.value->regions().size(), 2U);

  MemoryRegion zero_region{id(14), address(0x3000), UInt128(0), alignment(0x1000),
                           MemoryRegionKind::Reserved, "empty"};
  AvailableMemoryBlock zero_block{id(15), id(14), address(0x3000), UInt128(0),
                                  alignment(0x1000)};
  auto zero = AddressSpace::create(id(16), 64, {zero_region}, {zero_block});
  ck_assert_msg(zero, "zero-sized region and block rejected");
}
END_TEST

START_TEST(test_topology_rejects_invalid_alignment_containment_and_overlap)
{
  MemoryRegion bad_start{id(11), address(0x1001), UInt128(0x1000), alignment(0x1000),
                         MemoryRegionKind::Ram, "bad"};
  auto invalid_start = AddressSpace::create(id(10), 64, {bad_start}, {});
  ck_assert_msg(!invalid_start, "misaligned region start accepted");

  MemoryRegion bad_size{id(11), address(0x1000), UInt128(0x1001), alignment(0x1000),
                        MemoryRegionKind::Ram, "bad"};
  auto invalid_size = AddressSpace::create(id(10), 64, {bad_size}, {});
  ck_assert_msg(!invalid_size, "misaligned region size accepted");

  MemoryRegion region{id(11), address(0x1000), UInt128(0x2000), alignment(0x1000),
                      MemoryRegionKind::Ram, "ram"};
  AvailableMemoryBlock outside{id(12), id(11), address(0x3000), UInt128(1), std::nullopt};
  auto invalid_containment = AddressSpace::create(id(10), 64, {region}, {outside});
  ck_assert_msg(!invalid_containment, "out-of-region block accepted");

  AvailableMemoryBlock first{id(12), id(11), address(0x1000), UInt128(0x1000),
                             alignment(0x1000)};
  AvailableMemoryBlock second{id(13), id(11), address(0x1800), UInt128(0x800),
                              alignment(0x800)};
  auto invalid_overlap = AddressSpace::create(id(10), 64, {region}, {first, second});
  ck_assert_msg(!invalid_overlap, "overlapping available blocks accepted");

  AvailableMemoryBlock wrong_width{id(12), id(11), address(0x1000, 32), UInt128(0),
                                   std::nullopt};
  auto invalid_width = AddressSpace::create(id(10), 64, {region}, {wrong_width});
  ck_assert_msg(!invalid_width, "block with mismatched address width accepted");

  MemoryRegion beyond_32_bits{id(14), address(0xFFFFFFFFULL, 32), UInt128(2),
                              std::nullopt, MemoryRegionKind::Ram, "overflow"};
  auto invalid_range = AddressSpace::create(id(15), 32, {beyond_32_bits}, {});
  ck_assert_msg(!invalid_range, "range beyond address-space width accepted");
}
END_TEST

START_TEST(test_regions_may_overlap_with_explicit_block_parent)
{
  MemoryRegion ram{id(11), address(0x1000), UInt128(0x2000), std::nullopt,
                   MemoryRegionKind::Ram, "ram"};
  MemoryRegion firmware{id(12), address(0x1800), UInt128(0x800), std::nullopt,
                        MemoryRegionKind::Firmware, "firmware"};
  AvailableMemoryBlock block{id(13), id(12), address(0x1800), UInt128(0x800),
                             std::nullopt};
  auto space = AddressSpace::create(id(10), 64, {ram, firmware}, {block});
  ck_assert_msg(space, "overlapping regions with explicit block parent rejected");
  ck_assert_msg(space.value->available_blocks()[0].region_id == id(12),
                "block parent changed");
}
END_TEST

START_TEST(test_inventory_validates_references_and_orders_queries)
{
  MemoryRegion region{id(21), address(0), UInt128(0x4000), alignment(0x1000),
                      MemoryRegionKind::Ram, "ram"};
  AvailableMemoryBlock block{id(22), id(21), address(0), UInt128(0x4000),
                             alignment(0x1000)};
  auto space = address_space({region}, {block});
  CoreDescriptor later{id(32), id(1), id(2), 1, true};
  CoreDescriptor earlier{id(31), id(1), id(2), 0, true};
  RegisterFileDescriptor file{id(33), id(31), {id(3), id(4)}};
  BusDescriptor child{id(42), "pci", "child", id(41)};
  BusDescriptor root{id(41), "system", "root", std::nullopt};
  DeviceDescriptor device{id(43), "display", "gpu", id(42)};

  auto inventory = MachineInventory::create(
      {architecture()}, {later, earlier}, {file}, {std::move(space)},
      {child, root}, {device});
  ck_assert_msg(inventory, "valid inventory rejected: %s",
                inventory.error.has_value() ? inventory.error->message.c_str() : "ok");
  ck_assert_msg(inventory.value->cores()[0].resource_id == id(31),
                "core enumeration is not resource-ID ordered");
  ck_assert_msg(inventory.value->buses()[0].resource_id == id(41),
                "bus enumeration is not resource-ID ordered");
  ck_assert_ptr_nonnull(inventory.value->find_core(id(32)));
  ck_assert_ptr_nonnull(inventory.value->find_memory_region(id(21)));
  ck_assert_ptr_nonnull(inventory.value->find_available_memory_block(id(22)));
  ck_assert_ptr_nonnull(inventory.value->find_device(id(43)));
  ck_assert_ptr_null(inventory.value->find_device(id(99)));
  ck_assert_msg(inventory.value->memory_regions()[0].get().resource_id == id(21),
                "memory-region enumeration is not resource-ID ordered");

  auto dangling = MachineInventory::create(
      {architecture()}, {{id(31), id(1), id(99), 0, true}}, {}, {}, {}, {});
  ck_assert_msg(!dangling, "dangling core-definition reference accepted");

  auto duplicate = MachineInventory::create(
      {architecture()}, {{id(31), id(1), id(2), 0, true}},
      {{id(31), id(31), {id(3)}}}, {}, {}, {});
  ck_assert_msg(!duplicate, "duplicate cross-category resource ID accepted");

  auto bus_cycle = MachineInventory::create(
      {}, {}, {}, {},
      {{id(41), "pci", "first", id(42)}, {id(42), "pci", "second", id(41)}}, {});
  ck_assert_msg(!bus_cycle, "cyclic bus parents accepted");
}
END_TEST

START_TEST(test_machine_inventory_types_are_registered)
{
  referee::SqliteStore store(referee::SqliteConfig{.filename = ":memory:", .enable_wal = false});
  ck_assert_msg(store.open(), "store open failed");
  ck_assert_msg(store.ensure_schema(), "store schema failed");
  iris::refract::SchemaRegistry registry(store);
  auto bootstrap = iris::refract::bootstrap_core_schema(registry);
  ck_assert_msg(bootstrap, "bootstrap failed");

  const std::array<referee::TypeID, 13> expected = {
      kRegisterRoleType, kRegisterDefinitionType, kCoreDefinitionType,
      kArchitectureDefinitionType, kMemoryRegionKindType, kMemoryRegionType,
      kAvailableMemoryBlockType, kAddressSpaceType, kCoreDescriptorType,
      kRegisterFileDescriptorType, kBusDescriptorType, kDeviceDescriptorType,
      kMachineInventoryType};
  for (const auto type : expected) {
    auto definition = registry.get_definition_by_type(type);
    ck_assert_msg(definition && definition.value->has_value(),
                  "Machine inventory schema type missing");
  }
  ck_assert_msg(store.close(), "store close failed");
}
END_TEST

Suite* machine_inventory_suite(void) {
  Suite* suite = suite_create("MachineInventory");
  TCase* tests = tcase_create("core");
  tcase_add_test(tests, test_architecture_definitions_preserve_order_and_state_boundary);
  tcase_add_test(tests, test_topology_allows_zero_sizes_and_optional_alignment);
  tcase_add_test(tests, test_topology_rejects_invalid_alignment_containment_and_overlap);
  tcase_add_test(tests, test_regions_may_overlap_with_explicit_block_parent);
  tcase_add_test(tests, test_inventory_validates_references_and_orders_queries);
  tcase_add_test(tests, test_machine_inventory_types_are_registered);
  suite_add_tcase(suite, tests);
  return suite;
}

int main(void) {
  Suite* suite = machine_inventory_suite();
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
