extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "services/network_service.h"
#include "services/service_host.h"
#include "refract/bootstrap.h"

#include <nlohmann/json.hpp>

using namespace referee;
using namespace iris::service;
using namespace iris::machine;
using namespace iris::comms;
using nlohmann::json;

namespace {
struct Fixture {
  ObjectID owner = ObjectID::random(), context = ObjectID::random();
  ObjectID device = ObjectID::random(), lease = ObjectID::random(), bus = ObjectID::random();
  SqliteStore store{{.filename=":memory:"}};
  CapabilityContextStore contexts{store};
  MachineInventory inventory;
  MachineLeaseRegistry leases;
  iris::refract::SchemaRegistry schemas{store};
  Fixture() : inventory(std::move(*MachineInventory::create({}, {}, {}, {},
                              {{bus, "system", "test", std::nullopt}},
                              {{device, "loopback", "test", bus}}).value)),
              leases(inventory, contexts) {
    ck_assert(store.open());
    ck_assert(iris::refract::bootstrap_core_schema(schemas));
    ck_assert(contexts.persist_context({context, owner, std::nullopt,
        {{machine_capability_grant(MachineResourceKind::Device, MachineAccessMode::Control, device)}}}));
    MachineHandleFactory factory(inventory, contexts);
    auto handle = factory.create_device_handle(device, MachineAccessMode::Control, context);
    ck_assert(handle);
    ck_assert(leases.create_lease(lease, owner, MachineHandle{*handle.value}));
  }
  Protocol protocol(TransportSemantics semantics = TransportSemantics::Stream) {
    auto result = Protocol::create(ObjectID::random(), "loopback", semantics,
        {MachineResourceKind::Device}, {{MachineResourceKind::Device, MachineAccessMode::Control}});
    ck_assert(result);
    return std::move(*result.value);
  }
  Transport transport(bool authorized = true) {
    TransportFactory factory(inventory, leases);
    auto result = authorized ? factory.create_authorized_transport(ObjectID::random(),
        TransportSemantics::Stream, lease, owner, context) : factory.create_descriptor_transport(
        ObjectID::random(), TransportSemantics::Stream, MachineResourceKind::Device, device);
    ck_assert(result);
    return std::move(*result.value);
  }
};

MessageEnvelope request(NetworkService& service, ObjectID owner, TypeID operation, json payload) {
  auto result = make_request_to_object(owner, service.descriptor().id, operation, json::to_cbor(payload));
  result.endpoint = iris::service::Endpoint{"", operation, {}};
  result.sandbox = ObjectID::random();
  return result;
}
// The 1 MiB round-trip criterion has no latency SLA; allow slower CI runners.
json send(IpcService& ipc, MessageEnvelope envelope) {
  auto result = ipc.send_request(envelope, std::chrono::seconds(5));
  ck_assert_msg(result, "%s", result.error ? result.error->message.c_str() : "ok");
  ck_assert(result.value->recipient == envelope.sender);
  ck_assert(result.value->correlation_id == envelope.correlation_id);
  ck_assert(result.value->sandbox == envelope.sandbox);
  ck_assert(result.value->message_type == kNetworkResponseType);
  return json::from_cbor(result.value->payload_cbor);
}
void error(IpcService& ipc, MessageEnvelope envelope, ErrorCode code) {
  auto result = ipc.send_request(envelope, std::chrono::seconds(5));
  ck_assert(!result);
  ck_assert_msg(result.error->code == code, "%s", result.error->message.c_str());
}
json endpoints() {
  return {{"first_endpoint", ObjectID::random().to_hex()},
          {"second_endpoint", ObjectID::random().to_hex()}};
}
}

START_TEST(test_network_host_session_and_packet_lifecycle)
{
  Fixture fixture;
  auto transport = fixture.transport();
  NetworkService service(ObjectID::random(), fixture.schemas, fixture.leases,
                         fixture.protocol(), transport, 2);
  iris::ceo::TaskRegistry tasks;
  ServiceHostManager hosts(fixture.store, tasks);
  ck_assert(hosts.recover());
  ck_assert(hosts.create(service.descriptor()));
  ck_assert(hosts.bind(service.descriptor().id, service));
  ck_assert(hosts.start(service.descriptor().id));
  IpcService ipc(hosts.registry());
  auto input = endpoints();
  auto opened = send(ipc, request(service, fixture.owner, kNetworkOpenType, input));
  ck_assert(opened["state"] == "open");
  ck_assert(opened["transport"] == transport.id().to_hex());
  ck_assert(opened["first_endpoint"] == input["first_endpoint"]);
  ck_assert(opened["second_endpoint"] == input["second_endpoint"]);
  auto lookup = json{{"id", opened["id"]}};
  ck_assert(send(ipc, request(service, fixture.owner, kNetworkInspectType, lookup)) == opened);
  for (const auto& bytes : std::vector<Bytes>{{}, {0, 255, 0, 10}, Bytes(kMaximumFramePayload, 0xab)}) {
    auto response = send(ipc, request(service, fixture.owner, kNetworkRoundTripType,
                                     {{"id", opened["id"]}, {"payload", json::binary(bytes)}}));
    ck_assert(response["payload"] == json::binary(bytes));
  }
  auto closed = send(ipc, request(service, fixture.owner, kNetworkCloseType, lookup));
  ck_assert(closed["state"] == "closed");
  ck_assert(send(ipc, request(service, fixture.owner, kNetworkInspectType, lookup)) == closed);
  error(ipc, request(service, fixture.owner, kNetworkCloseType, lookup), ErrorCode::FailedPrecondition);
  error(ipc, request(service, fixture.owner, kNetworkRoundTripType,
                    {{"id", opened["id"]}, {"payload", json::binary(Bytes{1})}}),
        ErrorCode::FailedPrecondition);
  auto second = send(ipc, request(service, fixture.owner, kNetworkOpenType, endpoints()));
  ck_assert(second["id"] != opened["id"]);
  error(ipc, request(service, fixture.owner, kNetworkOpenType, endpoints()), ErrorCode::FailedPrecondition);
  ck_assert(hosts.stop(service.descriptor().id));
  ck_assert(hosts.unbind(service.descriptor().id));
}
END_TEST

START_TEST(test_network_rejects_cross_owner_and_revoked_authority)
{
  Fixture fixture;
  NetworkService service(ObjectID::random(), fixture.schemas, fixture.leases,
                         fixture.protocol(), fixture.transport(), 2);
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  error(ipc, request(service, ObjectID::random(), kNetworkOpenType, endpoints()), ErrorCode::FailedPrecondition);
  auto opened = send(ipc, request(service, fixture.owner, kNetworkOpenType, endpoints()));
  for (auto type : {kNetworkInspectType, kNetworkCloseType, kNetworkRoundTripType}) {
    json payload{{"id", opened["id"]}};
    if (type == kNetworkRoundTripType) payload["payload"] = json::binary(Bytes{1});
    error(ipc, request(service, ObjectID::random(), type, payload), ErrorCode::FailedPrecondition);
  }
  ck_assert(fixture.leases.revoke(fixture.lease));
  error(ipc, request(service, fixture.owner, kNetworkOpenType, endpoints()), ErrorCode::FailedPrecondition);
  for (auto type : {kNetworkInspectType, kNetworkCloseType, kNetworkRoundTripType}) {
    json payload{{"id", opened["id"]}};
    if (type == kNetworkRoundTripType) payload["payload"] = json::binary(Bytes{1});
    error(ipc, request(service, fixture.owner, type, payload), ErrorCode::FailedPrecondition);
  }
}
END_TEST

START_TEST(test_network_rejects_descriptor_and_incompatible_bindings)
{
  Fixture fixture;
  NetworkService descriptor(ObjectID::random(), fixture.schemas, fixture.leases,
                            fixture.protocol(), fixture.transport(false), 1);
  ck_assert(!descriptor.handle_message(request(descriptor, fixture.owner, kNetworkOpenType, endpoints())));
  NetworkService mismatch(ObjectID::random(), fixture.schemas, fixture.leases,
                          fixture.protocol(TransportSemantics::Datagram), fixture.transport(), 1);
  ck_assert(!mismatch.handle_message(request(mismatch, fixture.owner, kNetworkOpenType, endpoints())));
  NetworkService disabled(ObjectID::random(), fixture.schemas, fixture.leases,
                          fixture.protocol(), fixture.transport(), 0);
  ck_assert(!disabled.handle_message(request(disabled, fixture.owner, kNetworkOpenType, endpoints())));
}
END_TEST

START_TEST(test_network_invalid_requests_preserve_session_slot)
{
  Fixture fixture;
  NetworkService service(ObjectID::random(), fixture.schemas, fixture.leases,
                         fixture.protocol(), fixture.transport(), 1);
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  auto valid = request(service, fixture.owner, kNetworkOpenType, endpoints());
  for (auto payload : std::vector<json>{json::array(), json::object(),
        {{"first_endpoint", "bad"}, {"second_endpoint", 12}},
        {{"first_endpoint", ObjectID::random().to_hex()}, {"second_endpoint", ObjectID::random().to_hex()},
         {"owner", fixture.owner.to_hex()}}}) {
    error(ipc, request(service, fixture.owner, kNetworkOpenType, payload), ErrorCode::InvalidArgument);
  }
  auto bad = valid; bad.payload_cbor = {0xff}; error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint.reset(); error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint->name = "network.close"; error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.message_type = kNetworkCloseType; error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint->type = TypeID{1}; error(ipc, bad, ErrorCode::InvalidArgument);
  auto opened = send(ipc, valid);
  error(ipc, request(service, fixture.owner, kNetworkInspectType,
        {{"id", ObjectID::random().to_hex()}}), ErrorCode::NotFound);
  for (auto payload : std::vector<json>{nullptr, json::array({1}),
                                        json::binary(Bytes(kMaximumFramePayload + 1, 0))}) {
    error(ipc, request(service, fixture.owner, kNetworkRoundTripType,
          {{"id", opened["id"]}, {"payload", payload}}), ErrorCode::InvalidArgument);
  }
  ck_assert(send(ipc, request(service, fixture.owner, kNetworkInspectType,
        {{"id", opened["id"]}}))["state"] == "open");
}
END_TEST

int main() {
  auto* suite = suite_create("Network service");
  auto* tc = tcase_create("local sessions");
  tcase_set_timeout(tc, 30);
  tcase_add_test(tc, test_network_host_session_and_packet_lifecycle);
  tcase_add_test(tc, test_network_rejects_cross_owner_and_revoked_authority);
  tcase_add_test(tc, test_network_rejects_descriptor_and_incompatible_bindings);
  tcase_add_test(tc, test_network_invalid_requests_preserve_session_slot);
  suite_add_tcase(suite, tc);
  auto* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
