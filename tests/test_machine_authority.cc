extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "machine/authority.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace iris::machine;

namespace {

referee::ObjectID id(std::uint8_t value) {
  referee::ObjectID out{};
  out.bytes.back() = value;
  return out;
}

Address address(std::uint64_t value) {
  auto result = Address::create(UInt128(value), 64);
  ck_assert_msg(result, "address construction failed");
  return *result.value;
}

MachineInventory inventory() {
  MemoryRegion ram{id(10), address(0), UInt128(0x1000), std::nullopt,
                   MemoryRegionKind::Ram, "ram"};
  MemoryRegion io{id(11), address(0x1000), UInt128(0x1000), std::nullopt,
                  MemoryRegionKind::MemoryMapped, "io"};
  AvailableMemoryBlock block{id(12), id(10), address(0), UInt128(0x1000), std::nullopt};
  auto address_space = AddressSpace::create(id(13), 64, {ram, io}, {block});
  ck_assert_msg(address_space, "address-space construction failed");

  auto result = MachineInventory::create(
      {}, {}, {}, {std::move(*address_space.value)},
      {{id(20), "system", "root", std::nullopt}},
      {{id(21), "display", "gpu", id(20)}});
  ck_assert_msg(result, "inventory construction failed");
  return std::move(*result.value);
}

iris::service::CapabilityContext context_with(
    referee::ObjectID context_id,
    referee::ObjectID subject_id,
    std::vector<std::string> grants) {
  iris::service::CapabilityContext context;
  context.id = context_id;
  context.subject = subject_id;
  for (auto& grant : grants) {
    context.grants.push_back({std::move(grant)});
  }
  return context;
}

class CapabilityFixture {
public:
  CapabilityFixture()
      : store({.filename = ":memory:", .enable_wal = false}), contexts(store) {
    ck_assert_msg(store.open(), "capability store open failed");
    ck_assert_msg(store.ensure_schema(), "capability store schema failed");
  }

  ~CapabilityFixture() { store.close(); }

  void persist(const iris::service::CapabilityContext& context) {
    auto result = contexts.persist_context(context);
    ck_assert_msg(result, "capability context persistence failed");
  }

  referee::SqliteStore store;
  iris::service::CapabilityContextStore contexts;
};

} // namespace

START_TEST(test_handle_factory_requires_exact_resource_scoped_grants)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineHandleFactory factory(machine, fixture.contexts);
  const auto memory_grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  const auto device_grant = machine_capability_grant(
      MachineResourceKind::Device, MachineAccessMode::Control, id(21));
  const auto unknown_device_grant = machine_capability_grant(
      MachineResourceKind::Device, MachineAccessMode::Control, id(22));
  const auto io_grant = machine_capability_grant(
      MachineResourceKind::IoRegion, MachineAccessMode::Write, id(11));
  auto context = context_with(
      id(30), id(31), {memory_grant, device_grant, unknown_device_grant, io_grant});
  fixture.persist(context);

  ck_assert_str_eq(memory_grant.c_str(),
                   "machine.memory.read:0000000000000000000000000000000c");

  auto memory = factory.create_memory_handle(id(12), MachineAccessMode::Read, context.id);
  ck_assert_msg(memory, "authorized memory handle rejected");
  ck_assert_msg(memory.value->resource_id == id(12), "memory resource ID changed");
  ck_assert_msg(memory.value->capability_context_id == id(30), "context ID changed");

  auto device = factory.create_device_handle(id(21), MachineAccessMode::Control, context.id);
  ck_assert_msg(device, "authorized device handle rejected");

  auto io = factory.create_io_region_handle(id(11), MachineAccessMode::Write, context.id);
  ck_assert_msg(io, "authorized IO-region handle rejected");

  auto wrong_mode = factory.create_memory_handle(id(12), MachineAccessMode::Write, context.id);
  ck_assert_msg(!wrong_mode, "grant for a different access mode was accepted");
  ck_assert_int_eq(static_cast<int>(wrong_mode.error->code),
                   static_cast<int>(referee::ErrorCode::FailedPrecondition));

  auto wrong_resource = factory.create_device_handle(id(22), MachineAccessMode::Control, context.id);
  ck_assert_msg(!wrong_resource, "unknown device was accepted");
  ck_assert_int_eq(static_cast<int>(wrong_resource.error->code),
                   static_cast<int>(referee::ErrorCode::NotFound));

  auto forged = context_with(id(32), id(31), {memory_grant});
  auto unpersisted = factory.create_memory_handle(id(12), MachineAccessMode::Read, forged.id);
  ck_assert_msg(!unpersisted, "unpersisted capability context was accepted");
  ck_assert_int_eq(static_cast<int>(unpersisted.error->code),
                   static_cast<int>(referee::ErrorCode::NotFound));
}
END_TEST

START_TEST(test_memory_and_io_region_handle_boundaries_are_distinct)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineHandleFactory factory(machine, fixture.contexts);
  auto context = context_with(
      id(30), id(31),
      {machine_capability_grant(MachineResourceKind::Memory, MachineAccessMode::Read, id(10)),
       machine_capability_grant(MachineResourceKind::IoRegion,
                                MachineAccessMode::Read, id(10)),
       machine_capability_grant(MachineResourceKind::IoRegion,
                                MachineAccessMode::Read, id(11))});
  fixture.persist(context);

  auto region_as_memory = factory.create_memory_handle(id(10), MachineAccessMode::Read, context.id);
  ck_assert_msg(!region_as_memory, "MemoryHandle accepted a region instead of an available block");

  auto ram_as_io = factory.create_io_region_handle(id(10), MachineAccessMode::Read, context.id);
  ck_assert_msg(!ram_as_io, "IoRegionHandle accepted a non-memory-mapped region");
  ck_assert_int_eq(static_cast<int>(ram_as_io.error->code),
                   static_cast<int>(referee::ErrorCode::FailedPrecondition));

  auto mapped_io = factory.create_io_region_handle(id(11), MachineAccessMode::Read, context.id);
  ck_assert_msg(mapped_io, "IoRegionHandle rejected a memory-mapped region");
}
END_TEST

START_TEST(test_lease_creation_validates_owner_context_and_grant)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineHandleFactory factory(machine, fixture.contexts);
  const auto grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  auto context = context_with(id(30), id(31), {grant});
  fixture.persist(context);
  auto handle = factory.create_memory_handle(id(12), MachineAccessMode::Read, context.id);
  ck_assert_msg(handle, "handle construction failed");

  MachineLeaseRegistry leases(fixture.contexts);
  auto lease = leases.create_lease(id(40), id(31), MachineHandle{*handle.value});
  ck_assert_msg(lease, "valid lease rejected");
  ck_assert_int_eq(static_cast<int>(lease.value->state()),
                   static_cast<int>(MachineLeaseState::Active));
  ck_assert_msg(leases.authorize_use(id(40), id(31), id(30)), "active lease denied use");

  auto wrong_owner = leases.create_lease(id(41), id(32), MachineHandle{*handle.value});
  ck_assert_msg(!wrong_owner, "owner mismatch accepted");

  MemoryHandle forged{id(12), MachineAccessMode::Read, id(33)};
  auto missing_context = leases.create_lease(id(42), id(31), MachineHandle{forged});
  ck_assert_msg(!missing_context, "handle with an unknown context was accepted");
  ck_assert_int_eq(static_cast<int>(missing_context.error->code),
                   static_cast<int>(referee::ErrorCode::NotFound));

  auto duplicate = leases.create_lease(id(40), id(31), MachineHandle{*handle.value});
  ck_assert_msg(!duplicate, "duplicate lease ID accepted");
  ck_assert_int_eq(static_cast<int>(duplicate.error->code),
                   static_cast<int>(referee::ErrorCode::AlreadyExists));
}
END_TEST

START_TEST(test_release_is_owner_checked_and_terminal)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineHandleFactory factory(machine, fixture.contexts);
  const auto grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  auto context = context_with(id(30), id(31), {grant});
  fixture.persist(context);
  auto handle = factory.create_memory_handle(id(12), MachineAccessMode::Read, context.id);
  MachineLeaseRegistry leases(fixture.contexts);
  ck_assert_msg(leases.create_lease(id(40), id(31), MachineHandle{*handle.value}),
                "lease construction failed");

  auto mismatch = leases.release(id(40), id(32));
  ck_assert_msg(!mismatch, "non-owner released lease");
  ck_assert_int_eq(static_cast<int>(leases.find(id(40))->state()),
                   static_cast<int>(MachineLeaseState::Active));

  ck_assert_msg(leases.release(id(40), id(31)), "owner release failed");
  ck_assert_int_eq(static_cast<int>(leases.find(id(40))->state()),
                   static_cast<int>(MachineLeaseState::Released));
  ck_assert_msg(!leases.authorize_use(id(40), id(31), id(30)),
                "released lease authorized use");
  ck_assert_msg(!leases.release(id(40), id(31)), "second release succeeded");
  ck_assert_msg(!leases.revoke(id(40)), "released lease changed to revoked");
}
END_TEST

START_TEST(test_supervisor_revocation_does_not_require_live_owner_or_context_objects)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineHandleFactory factory(machine, fixture.contexts);
  const auto memory_grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  const auto device_grant = machine_capability_grant(
      MachineResourceKind::Device, MachineAccessMode::Control, id(21));
  auto context = context_with(id(30), id(31), {memory_grant, device_grant});
  fixture.persist(context);
  auto memory = factory.create_memory_handle(id(12), MachineAccessMode::Read, context.id);
  auto device = factory.create_device_handle(id(21), MachineAccessMode::Control, context.id);
  MachineLeaseRegistry leases(fixture.contexts);
  ck_assert_msg(leases.create_lease(id(40), id(31), MachineHandle{*memory.value}),
                "memory lease construction failed");
  ck_assert_msg(leases.create_lease(id(41), id(31), MachineHandle{*device.value}),
                "device lease construction failed");

  context = {};
  ck_assert_uint_eq(leases.revoke_for_context(id(30)), 2U);
  ck_assert_int_eq(static_cast<int>(leases.find(id(40))->state()),
                   static_cast<int>(MachineLeaseState::Revoked));
  ck_assert_int_eq(static_cast<int>(leases.find(id(41))->state()),
                   static_cast<int>(MachineLeaseState::Revoked));
  ck_assert_uint_eq(leases.revoke_for_context(id(30)), 0U);
  ck_assert_msg(!leases.authorize_use(id(41), id(31), id(30)),
                "revoked lease authorized use");
}
END_TEST

START_TEST(test_supervisor_can_revoke_one_lease_or_all_leases_for_owner)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineHandleFactory factory(machine, fixture.contexts);
  const auto grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  auto context = context_with(id(30), id(31), {grant});
  fixture.persist(context);
  auto handle = factory.create_memory_handle(id(12), MachineAccessMode::Read, context.id);
  MachineLeaseRegistry leases(fixture.contexts);
  ck_assert_msg(leases.create_lease(id(40), id(31), MachineHandle{*handle.value}),
                "first lease construction failed");
  ck_assert_msg(leases.create_lease(id(41), id(31), MachineHandle{*handle.value}),
                "second lease construction failed");

  ck_assert_msg(leases.revoke(id(40)), "single-lease revocation failed");
  ck_assert_int_eq(static_cast<int>(leases.find(id(40))->state()),
                   static_cast<int>(MachineLeaseState::Revoked));
  ck_assert_uint_eq(leases.revoke_for_owner(id(31)), 1U);
  ck_assert_int_eq(static_cast<int>(leases.find(id(41))->state()),
                   static_cast<int>(MachineLeaseState::Revoked));
  ck_assert_uint_eq(leases.revoke_for_owner(id(31)), 0U);
}
END_TEST

Suite* machine_authority_suite(void) {
  Suite* suite = suite_create("MachineAuthority");
  TCase* tests = tcase_create("core");
  tcase_add_test(tests, test_handle_factory_requires_exact_resource_scoped_grants);
  tcase_add_test(tests, test_memory_and_io_region_handle_boundaries_are_distinct);
  tcase_add_test(tests, test_lease_creation_validates_owner_context_and_grant);
  tcase_add_test(tests, test_release_is_owner_checked_and_terminal);
  tcase_add_test(tests,
                 test_supervisor_revocation_does_not_require_live_owner_or_context_objects);
  tcase_add_test(tests, test_supervisor_can_revoke_one_lease_or_all_leases_for_owner);
  suite_add_tcase(suite, tests);
  return suite;
}

int main(void) {
  Suite* suite = machine_authority_suite();
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
