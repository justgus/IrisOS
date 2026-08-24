extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "comms/primitives.h"
#include "comms/transport_session.h"
#include "refract/bootstrap.h"

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

START_TEST(test_protocol_metadata_is_validated_and_inspectable)
{
  auto protocol = Protocol::create(
      id(60), "registered-packet", TransportSemantics::Stream,
      {MachineResourceKind::Device, MachineResourceKind::Memory,
       MachineResourceKind::Device});
  ck_assert_msg(protocol, "valid protocol rejected");
  ck_assert_msg(protocol.value->id() == id(60), "protocol ID changed");
  ck_assert_str_eq(protocol.value->name().c_str(), "registered-packet");
  ck_assert_int_eq(static_cast<int>(protocol.value->required_semantics()),
                   static_cast<int>(TransportSemantics::Stream));
  ck_assert_uint_eq(protocol.value->allowed_resource_kinds().size(), 2U);

  ck_assert_msg(!Protocol::create(
                    id(61), "", TransportSemantics::Stream,
                    {MachineResourceKind::Memory}),
                "empty protocol name accepted");
  ck_assert_msg(!Protocol::create(id(62), "empty", TransportSemantics::Stream, {}),
                "protocol without resource kinds accepted");
}
END_TEST

START_TEST(test_protocol_compatibility_reports_deterministic_reasons)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  MachineLeaseRegistry leases(fixture.contexts);
  TransportFactory factory(machine, leases);
  auto protocol = Protocol::create(
      id(60), "registered-packet", TransportSemantics::Stream,
      {MachineResourceKind::Memory});
  auto memory = factory.create_descriptor_transport(
      id(61), TransportSemantics::Stream, MachineResourceKind::Memory, id(12));
  auto datagram = factory.create_descriptor_transport(
      id(62), TransportSemantics::Datagram, MachineResourceKind::Memory, id(12));
  auto device = factory.create_descriptor_transport(
      id(63), TransportSemantics::Stream, MachineResourceKind::Device, id(21));

  auto accepted = check_compatibility(*protocol.value, *memory.value, leases);
  ck_assert_msg(accepted.compatible, "compatible transport rejected");
  ck_assert_int_eq(static_cast<int>(accepted.reason),
                   static_cast<int>(CompatibilityReason::Compatible));
  ck_assert_msg(accepted.resource_kind == MachineResourceKind::Memory,
                "accepted resource kind not reported");

  auto wrong_semantics = check_compatibility(*protocol.value, *datagram.value, leases);
  ck_assert_msg(!wrong_semantics.compatible, "wrong transport semantics accepted");
  ck_assert_int_eq(static_cast<int>(wrong_semantics.reason),
                   static_cast<int>(CompatibilityReason::TransportSemanticsMismatch));

  auto wrong_resource = check_compatibility(*protocol.value, *device.value, leases);
  ck_assert_msg(!wrong_resource.compatible, "disallowed resource kind accepted");
  ck_assert_int_eq(static_cast<int>(wrong_resource.reason),
                   static_cast<int>(CompatibilityReason::ResourceKindNotAllowed));
  ck_assert_msg(wrong_resource.resource_kind == MachineResourceKind::Device,
                "rejected resource kind not reported");
}
END_TEST

START_TEST(test_authorized_transport_compatibility_revalidates_lease)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  const auto grant = machine_capability_grant(
      MachineResourceKind::Memory, MachineAccessMode::Read, id(12));
  fixture.persist(context_with(id(70), id(71), {grant}));
  MachineHandleFactory handles(machine, fixture.contexts);
  auto handle = handles.create_memory_handle(id(12), MachineAccessMode::Read, id(70));
  MachineLeaseRegistry leases(fixture.contexts);
  ck_assert_msg(leases.create_lease(id(72), id(71), MachineHandle{*handle.value}),
                "lease construction failed");
  TransportFactory factory(machine, leases);
  auto transport = factory.create_authorized_transport(
      id(73), TransportSemantics::Stream, id(72), id(71), id(70));
  auto protocol = Protocol::create(
      id(74), "registered-packet", TransportSemantics::Stream,
      {MachineResourceKind::Memory});

  auto active = check_compatibility(*protocol.value, *transport.value, leases);
  ck_assert_msg(active.compatible, "active lease rejected");
  ck_assert_msg(leases.revoke(id(72)), "lease revocation failed");
  auto revoked = check_compatibility(*protocol.value, *transport.value, leases);
  ck_assert_msg(!revoked.compatible, "revoked lease accepted");
  ck_assert_int_eq(static_cast<int>(revoked.reason),
                   static_cast<int>(CompatibilityReason::LeaseAuthorizationFailed));
}
END_TEST

START_TEST(test_single_frame_encoding_is_bounded_and_exact)
{
  Packet packet(Blob({Byte(0xaa), Byte(0xbb), Byte(0xcc)}));
  auto frame = encode_frame(packet);
  ck_assert_msg(frame, "valid packet framing failed");
  const Bytes expected{0x00, 0x00, 0x00, 0x03, 0xaa, 0xbb, 0xcc};
  ck_assert_msg(*frame.value == expected, "frame is not big-endian length-prefixed");
  auto decoded = decode_frame(*frame.value);
  ck_assert_msg(decoded, "valid frame rejected");
  ck_assert_msg(*decoded.value == packet, "decoded packet changed");

  ck_assert_msg(!decode_frame({0x00, 0x00, 0x00}), "truncated header accepted");
  ck_assert_msg(!decode_frame({0x00, 0x00, 0x00, 0x02, 0xaa}),
                "truncated payload accepted");
  ck_assert_msg(!decode_frame({0x00, 0x00, 0x00, 0x01, 0xaa, 0xbb}),
                "trailing bytes accepted");
  ck_assert_msg(!decode_frame({0x00, 0x10, 0x00, 0x01}),
                "oversized declaration accepted");

  std::vector<Byte> oversized(kMaximumFramePayload + 1U, Byte(0));
  ck_assert_msg(!encode_frame(Packet(Blob(std::move(oversized)))),
                "oversized payload accepted");
}
END_TEST

START_TEST(test_registered_packet_executes_over_open_compatible_loopback)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  iris::refract::SchemaRegistry schemas(fixture.store);
  ck_assert_msg(iris::refract::bootstrap_core_schema(schemas), "schema bootstrap failed");
  MachineLeaseRegistry leases(fixture.contexts);
  TransportFactory factory(machine, leases);
  auto transport = factory.create_descriptor_transport(
      id(80), TransportSemantics::Stream, MachineResourceKind::Memory, id(12));
  auto protocol = Protocol::create(
      id(81), "registered-packet", TransportSemantics::Stream,
      {MachineResourceKind::Memory});
  Session session(id(82), Endpoint{id(83)}, Endpoint{id(84)}, transport.value->id());
  ck_assert_msg(session.open(), "session open failed");
  Packet packet(Blob({Byte(0x10), Byte(0x20), Byte(0x30)}));

  auto round_trip = execute_registered_packet_round_trip(
      schemas, leases, *protocol.value, *transport.value, session, packet);
  ck_assert_msg(round_trip, "registered packet round trip failed");
  ck_assert_msg(*round_trip.value == packet, "round-trip packet changed");
}
END_TEST

START_TEST(test_packet_execution_rejects_unregistered_and_invalid_sessions)
{
  const auto machine = inventory();
  CapabilityFixture fixture;
  iris::refract::SchemaRegistry schemas(fixture.store);
  MachineLeaseRegistry leases(fixture.contexts);
  TransportFactory factory(machine, leases);
  auto transport = factory.create_descriptor_transport(
      id(90), TransportSemantics::Stream, MachineResourceKind::Memory, id(12));
  auto protocol = Protocol::create(
      id(91), "registered-packet", TransportSemantics::Stream,
      {MachineResourceKind::Memory});
  Session session(id(92), Endpoint{id(93)}, Endpoint{id(94)}, transport.value->id());
  Packet packet(Blob({Byte(0x10)}));

  ck_assert_msg(!execute_registered_packet_round_trip(
                    schemas, leases, *protocol.value, *transport.value, session, packet),
                "unregistered packet type accepted");
  ck_assert_msg(iris::refract::bootstrap_core_schema(schemas), "schema bootstrap failed");
  ck_assert_msg(!execute_registered_packet_round_trip(
                    schemas, leases, *protocol.value, *transport.value, session, packet),
                "created session accepted");

  Session wrong_transport(id(95), Endpoint{id(96)}, Endpoint{id(97)}, id(98));
  ck_assert_msg(wrong_transport.open(), "session open failed");
  ck_assert_msg(!execute_registered_packet_round_trip(
                    schemas, leases, *protocol.value, *transport.value,
                    wrong_transport, packet),
                "session with different transport accepted");

  auto datagram_transport = factory.create_descriptor_transport(
      id(99), TransportSemantics::Datagram, MachineResourceKind::Memory, id(12));
  auto datagram_protocol = Protocol::create(
      id(100), "datagram-packet", TransportSemantics::Datagram,
      {MachineResourceKind::Memory});
  Session datagram_session(
      id(101), Endpoint{id(102)}, Endpoint{id(103)}, datagram_transport.value->id());
  ck_assert_msg(datagram_session.open(), "datagram session open failed");
  ck_assert_msg(!execute_registered_packet_round_trip(
                    schemas, leases, *datagram_protocol.value,
                    *datagram_transport.value, datagram_session, packet),
                "datagram execution accepted");
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
  tcase_add_test(tests, test_protocol_metadata_is_validated_and_inspectable);
  tcase_add_test(tests, test_protocol_compatibility_reports_deterministic_reasons);
  tcase_add_test(tests, test_authorized_transport_compatibility_revalidates_lease);
  tcase_add_test(tests, test_single_frame_encoding_is_bounded_and_exact);
  tcase_add_test(tests, test_registered_packet_executes_over_open_compatible_loopback);
  tcase_add_test(tests, test_packet_execution_rejects_unregistered_and_invalid_sessions);
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
