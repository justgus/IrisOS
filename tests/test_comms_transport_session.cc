extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "comms/primitives.h"
#include "comms/transport_session.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace iris::comms;
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
  for (auto& grant : grants) context.grants.push_back({std::move(grant)});
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
    ck_assert_msg(contexts.persist_context(context), "capability context persistence failed");
  }

  referee::SqliteStore store;
  iris::service::CapabilityContextStore contexts;
};

} // namespace

START_TEST(test_descriptor_transports_validate_machine_resource_categories)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineLeaseRegistry leases(fixture.contexts);
  TransportFactory factory(machine, leases);

  auto memory = factory.create_descriptor_transport(
      id(30), TransportSemantics::Stream, MachineResourceKind::Memory, id(12));
  ck_assert_msg(memory, "available-memory transport rejected");
  ck_assert_int_eq(static_cast<int>(memory.value->semantics()),
                   static_cast<int>(TransportSemantics::Stream));
  const auto* memory_resource = std::get_if<DescriptorTransportResource>(
      &memory.value->resource());
  ck_assert_ptr_nonnull(memory_resource);
  ck_assert_msg(memory_resource->resource_id == id(12), "resource ID changed");

  auto device = factory.create_descriptor_transport(
      id(31), TransportSemantics::Datagram, MachineResourceKind::Device, id(21));
  ck_assert_msg(device, "device transport rejected");

  auto io = factory.create_descriptor_transport(
      id(32), TransportSemantics::Datagram, MachineResourceKind::IoRegion, id(11));
  ck_assert_msg(io, "memory-mapped IO transport rejected");

  auto ram_as_io = factory.create_descriptor_transport(
      id(33), TransportSemantics::Stream, MachineResourceKind::IoRegion, id(10));
  ck_assert_msg(!ram_as_io, "RAM region accepted as IO transport resource");
  ck_assert_int_eq(static_cast<int>(ram_as_io.error->code),
                   static_cast<int>(referee::ErrorCode::NotFound));
}
END_TEST

START_TEST(test_authorized_transport_requires_matching_active_lease)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  const auto grant = machine_capability_grant(
      MachineResourceKind::Device, MachineAccessMode::Control, id(21));
  const auto context = context_with(id(40), id(41), {grant});
  fixture.persist(context);
  MachineHandleFactory handles(machine, fixture.contexts);
  auto handle = handles.create_device_handle(id(21), MachineAccessMode::Control, id(40));
  ck_assert_msg(handle, "authorized handle construction failed");
  MachineLeaseRegistry leases(fixture.contexts);
  ck_assert_msg(leases.create_lease(id(42), id(41), MachineHandle{*handle.value}),
                "lease construction failed");
  TransportFactory factory(machine, leases);

  auto transport = factory.create_authorized_transport(
      id(43), TransportSemantics::Datagram, id(42), id(41), id(40));
  ck_assert_msg(transport, "active authorized transport rejected");
  const auto* resource = std::get_if<AuthorizedTransportResource>(
      &transport.value->resource());
  ck_assert_ptr_nonnull(resource);
  ck_assert_msg(resource->lease_id == id(42), "lease ID changed");

  ck_assert_msg(!factory.create_authorized_transport(
                    id(44), TransportSemantics::Datagram, id(42), id(45), id(40)),
                "owner mismatch accepted");
  ck_assert_msg(!factory.create_authorized_transport(
                    id(45), TransportSemantics::Datagram, id(42), id(41), id(46)),
                "context mismatch accepted");
}
END_TEST

START_TEST(test_released_or_revoked_lease_rejects_transport_construction)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  const auto grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  const auto context = context_with(id(40), id(41), {grant});
  fixture.persist(context);
  MachineHandleFactory handles(machine, fixture.contexts);
  auto handle = handles.create_memory_handle(id(12), MachineAccessMode::Read, id(40));
  MachineLeaseRegistry leases(fixture.contexts);
  ck_assert_msg(leases.create_lease(id(42), id(41), MachineHandle{*handle.value}),
                "first lease construction failed");
  ck_assert_msg(leases.create_lease(id(43), id(41), MachineHandle{*handle.value}),
                "second lease construction failed");
  TransportFactory factory(machine, leases);

  ck_assert_msg(leases.release(id(42), id(41)), "lease release failed");
  ck_assert_msg(!factory.create_authorized_transport(
                    id(44), TransportSemantics::Stream, id(42), id(41), id(40)),
                "released lease accepted");

  ck_assert_msg(leases.revoke(id(43)), "lease revocation failed");
  ck_assert_msg(!factory.create_authorized_transport(
                    id(45), TransportSemantics::Stream, id(43), id(41), id(40)),
                "revoked lease accepted");
}
END_TEST

START_TEST(test_session_records_endpoint_and_transport_references)
{
  Session session(id(50), Endpoint{id(51)}, Endpoint{id(52)}, id(53));
  ck_assert_msg(session.id() == id(50), "session ID changed");
  ck_assert_msg(session.first_endpoint_id() == id(51), "first endpoint changed");
  ck_assert_msg(session.second_endpoint_id() == id(52), "second endpoint changed");
  ck_assert_msg(session.transport_id() == id(53), "transport changed");
  ck_assert_int_eq(static_cast<int>(session.state()),
                   static_cast<int>(SessionState::Created));
}
END_TEST

START_TEST(test_session_uses_strict_terminal_lifecycle)
{
  Session session(id(50), Endpoint{id(51)}, Endpoint{id(52)}, id(53));
  ck_assert_msg(!session.begin_close(), "Created session entered Closing");
  ck_assert_msg(!session.finish_close(), "Created session entered Closed");
  ck_assert_int_eq(static_cast<int>(session.state()),
                   static_cast<int>(SessionState::Created));

  ck_assert_msg(session.open(), "Created to Open failed");
  ck_assert_msg(!session.open(), "second open succeeded");
  ck_assert_int_eq(static_cast<int>(session.state()),
                   static_cast<int>(SessionState::Open));

  ck_assert_msg(session.begin_close(), "Open to Closing failed");
  ck_assert_msg(!session.open(), "Closing session reopened");
  ck_assert_msg(session.finish_close(), "Closing to Closed failed");
  ck_assert_msg(!session.finish_close(), "Closed session closed again");
  ck_assert_int_eq(static_cast<int>(session.state()),
                   static_cast<int>(SessionState::Closed));
}
END_TEST

START_TEST(test_existing_loopback_data_paths_remain_unchanged)
{
  auto channels = Channel::loopback();
  ck_assert_msg(channels.first.send({0x10, 0x20}).ready, "stream send failed");
  const auto stream_payload = channels.second.recv(2);
  ck_assert_uint_eq(stream_payload.size(), 2U);

  auto datagrams = DatagramPort::loopback();
  ck_assert_msg(datagrams.first.send({0x30, 0x40}).ready, "datagram send failed");
  const auto datagram_payload = datagrams.second.recv();
  ck_assert_msg(datagram_payload.has_value(), "datagram receive failed");
  ck_assert_uint_eq(datagram_payload->size(), 2U);
}
END_TEST

Suite* comms_transport_session_suite(void) {
  Suite* suite = suite_create("CommsTransportSession");
  TCase* tests = tcase_create("core");
  tcase_add_test(tests, test_descriptor_transports_validate_machine_resource_categories);
  tcase_add_test(tests, test_authorized_transport_requires_matching_active_lease);
  tcase_add_test(tests, test_released_or_revoked_lease_rejects_transport_construction);
  tcase_add_test(tests, test_session_records_endpoint_and_transport_references);
  tcase_add_test(tests, test_session_uses_strict_terminal_lifecycle);
  tcase_add_test(tests, test_existing_loopback_data_paths_remain_unchanged);
  suite_add_tcase(suite, tests);
  return suite;
}

int main(void) {
  Suite* suite = comms_transport_session_suite();
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
