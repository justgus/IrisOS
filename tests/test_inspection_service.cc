extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "services/inspection_service.h"
#include "services/service_host.h"

#include <filesystem>
#include <nlohmann/json.hpp>

using namespace referee;
using namespace iris::service;
using nlohmann::json;

namespace {
MessageEnvelope request(InspectionService& service, ObjectID sender, TypeID operation,
                        json payload = json::object()) {
  auto out = make_request_to_object(sender, service.descriptor().id, operation, json::to_cbor(payload));
  out.endpoint = Endpoint{"", operation, {}};
  out.sandbox = ObjectID::random();
  return out;
}
json send(IpcService& ipc, const MessageEnvelope& envelope) {
  auto result = ipc.send_request(envelope, std::chrono::milliseconds(10));
  ck_assert_msg(result, "%s", result.error ? result.error->message.c_str() : "ok");
  ck_assert(result.value->recipient == envelope.sender);
  ck_assert(result.value->correlation_id == envelope.correlation_id);
  ck_assert(result.value->sandbox == envelope.sandbox);
  ck_assert(result.value->message_type == kInspectionResponseType);
  return json::from_cbor(result.value->payload_cbor);
}
void error(IpcService& ipc, const MessageEnvelope& envelope, ErrorCode expected) {
  auto result = ipc.send_request(envelope, std::chrono::milliseconds(10));
  ck_assert(!result);
  ck_assert_msg(result.error->code == expected, "%s", result.error->message.c_str());
}
void seed(CapabilityContextStore& contexts, ObjectID owner, ObjectID other) {
  auto shared = ObjectID::random();
  ck_assert(contexts.persist_sandbox({shared, "shared", {owner, other}}));
  ck_assert(contexts.persist_sandbox({ObjectID::random(), "private-other", {other}}));
  ck_assert(contexts.persist_context({ObjectID::random(), owner, shared, {{"z"}, {"a"}}}));
  ck_assert(contexts.persist_context({ObjectID::random(), owner, std::nullopt, {{"b"}}}));
  ck_assert(contexts.persist_context({ObjectID::random(), other, std::nullopt, {{"secret-other"}}}));
}
}

START_TEST(test_inspection_filters_order_and_has_no_mutations)
{
  SqliteStore store({.filename=":memory:"});
  ck_assert(store.open());
  CapabilityContextStore contexts(store);
  auto owner = ObjectID::random(), other = ObjectID::random();
  seed(contexts, owner, other);
  auto before_contexts = store.list_by_type(kCapabilityContextType);
  auto before_sandboxes = store.list_by_type(kSandboxIdentityType);
  InspectionService service(ObjectID::random(), contexts);
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  auto output = send(ipc, request(service, owner, kInspectionContextsType));
  auto expected = contexts.list_contexts_for_subject(owner);
  ck_assert_uint_eq(output["contexts"].size(), 2);
  for (std::size_t i = 0; i < expected.value->size(); ++i) {
    const auto& context = (*expected.value)[i].context;
    const auto& item = output["contexts"][i];
    ck_assert(item["id"] == context.id.to_hex());
    ck_assert(item["subject"] == owner.to_hex());
    ck_assert(item["sandbox"] == (context.sandbox ? json(context.sandbox->to_hex()) : json(nullptr)));
    ck_assert_uint_eq(item["grants"].size(), context.grants.size());
    for (std::size_t g = 0; g < context.grants.size(); ++g) {
      ck_assert(item["grants"][g] == context.grants[g].name);
    }
  }
  auto sandboxes = send(ipc, request(service, owner, kInspectionSandboxesType));
  auto membership = contexts.list_sandboxes_for_subject(owner);
  ck_assert_uint_eq(sandboxes["sandboxes"].size(), 1);
  ck_assert(sandboxes["sandboxes"][0]["id"] == membership.value->front().sandbox.id.to_hex());
  ck_assert(sandboxes["sandboxes"][0]["name"] == "shared");
  ck_assert(sandboxes["sandboxes"][0]["subjects"] == json::array({owner.to_hex()}));
  ck_assert(output.dump().find("secret-other") == std::string::npos);
  ck_assert(sandboxes.dump().find(other.to_hex()) == std::string::npos);
  ck_assert(sandboxes.dump().find("private-other") == std::string::npos);
  auto stranger = ObjectID::random();
  ck_assert(send(ipc, request(service, stranger, kInspectionContextsType))["contexts"].empty());
  ck_assert(send(ipc, request(service, stranger, kInspectionSandboxesType))["sandboxes"].empty());
  auto after_contexts = store.list_by_type(kCapabilityContextType);
  auto after_sandboxes = store.list_by_type(kSandboxIdentityType);
  ck_assert_uint_eq(before_contexts.value->size(), after_contexts.value->size());
  ck_assert_uint_eq(before_sandboxes.value->size(), after_sandboxes.value->size());
  for (std::size_t i=0; i<before_contexts.value->size(); ++i) {
    ck_assert((*before_contexts.value)[i].ref == (*after_contexts.value)[i].ref);
    ck_assert((*before_contexts.value)[i].payload_cbor == (*after_contexts.value)[i].payload_cbor);
  }
  for (std::size_t i=0; i<before_sandboxes.value->size(); ++i) {
    ck_assert((*before_sandboxes.value)[i].ref == (*after_sandboxes.value)[i].ref);
    ck_assert((*before_sandboxes.value)[i].payload_cbor == (*after_sandboxes.value)[i].payload_cbor);
  }
  // Every request reads persisted data rather than a binding-time cache.
  ck_assert(contexts.persist_context({ObjectID::random(), owner, std::nullopt, {{"new"}}}));
  ck_assert_uint_eq(send(ipc, request(service, owner, kInspectionContextsType))["contexts"].size(), 3);
  ck_assert_uint_eq(send(ipc, request(service, other, kInspectionContextsType))["contexts"].size(), 1);
}
END_TEST

START_TEST(test_inspection_rejects_overrides_and_bad_selectors)
{
  SqliteStore store({.filename=":memory:"});
  ck_assert(store.open());
  CapabilityContextStore contexts(store);
  auto owner = ObjectID::random(), other = ObjectID::random();
  seed(contexts, owner, other);
  InspectionService service(ObjectID::random(), contexts);
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  for (auto operation : {kInspectionContextsType, kInspectionSandboxesType}) {
    for (auto payload : std::vector<json>{json::array(), nullptr, "", 1,
               {{"subject", other.to_hex()}}, {{"grants", json::array({"admin"})}},
               {{"sandbox", ObjectID::random().to_hex()}}}) {
      error(ipc, request(service, owner, operation, payload), ErrorCode::InvalidArgument);
    }
    auto valid = request(service, owner, operation);
    auto bad = valid; bad.payload_cbor = {0xff}; error(ipc, bad, ErrorCode::InvalidArgument);
    bad = valid; bad.endpoint.reset(); error(ipc, bad, ErrorCode::InvalidArgument);
    bad = valid; bad.endpoint = Endpoint{}; error(ipc, bad, ErrorCode::InvalidArgument);
    bad = valid; bad.endpoint->name = "inspection.mutate"; error(ipc, bad, ErrorCode::InvalidArgument);
    bad = valid; bad.endpoint->type = TypeID{1}; error(ipc, bad, ErrorCode::InvalidArgument);
    bad = valid; bad.message_type = TypeID{1}; error(ipc, bad, ErrorCode::InvalidArgument);
    bad = valid; bad.endpoint->name = operation == kInspectionContextsType ? "inspection.sandboxes" : "inspection.contexts";
    error(ipc, bad, ErrorCode::InvalidArgument);
  }
  auto by_name = request(service, owner, kInspectionContextsType);
  by_name.endpoint = Endpoint{"inspection.contexts", std::nullopt, {}};
  ck_assert_uint_eq(send(ipc, by_name)["contexts"].size(), 2);
  ck_assert_uint_eq(contexts.list_contexts().value->size(), 3);
  ck_assert_uint_eq(contexts.list_sandboxes().value->size(), 2);
}
END_TEST

START_TEST(test_inspection_propagates_corruption_and_storage_errors)
{
  for (bool sandbox : {false, true}) {
    SqliteStore store({.filename=":memory:"});
    ck_assert(store.open());
    CapabilityContextStore contexts(store);
    InspectionService service(ObjectID::random(), contexts);
    ServiceRegistry registry;
    ck_assert(registry.register_service(service.descriptor(), &service));
    IpcService ipc(registry);
    const auto operation = sandbox ? kInspectionSandboxesType : kInspectionContextsType;
    ck_assert(store.create_object_with_id(ObjectID::random(),
        sandbox ? kSandboxIdentityType : kCapabilityContextType, ObjectID{}, {0xff}));
    error(ipc, request(service, ObjectID::random(), operation), ErrorCode::CorruptData);
    ck_assert(store.close());
    error(ipc, request(service, ObjectID::random(), operation), ErrorCode::FailedPrecondition);
  }
}
END_TEST

START_TEST(test_inspection_host_reopen_and_explicit_binding)
{
  auto directory = std::filesystem::path("inspection-recovery-" + ObjectID::random().to_hex());
  std::filesystem::create_directory(directory);
  auto path = (directory / "store").string();
  auto id = ObjectID::random(), owner = ObjectID::random(), other = ObjectID::random();
  json before_contexts, before_sandboxes;
  for (int pass = 0; pass < 2; ++pass) {
    SqliteStore store({.filename=path});
    ck_assert(store.open());
    CapabilityContextStore contexts(store);
    if (pass == 0) seed(contexts, owner, other);
    InspectionService service(id, contexts);
    iris::ceo::TaskRegistry tasks;
    ServiceHostManager hosts(store, tasks);
    ck_assert(hosts.recover());
    if (pass == 0) ck_assert(hosts.create(service.descriptor()));
    else {
      ck_assert(hosts.inspect(id).value->state == ServiceHostState::Stopped);
      ck_assert(!hosts.inspect(id).value->bound);
      ck_assert(!hosts.start(id));
    }
    ck_assert(hosts.bind(id, service));
    IpcService ipc(hosts.registry());
    error(ipc, request(service, owner, kInspectionContextsType), ErrorCode::FailedPrecondition);
    ck_assert(hosts.start(id));
    auto current_contexts = send(ipc, request(service, owner, kInspectionContextsType));
    auto current_sandboxes = send(ipc, request(service, owner, kInspectionSandboxesType));
    if (pass == 0) { before_contexts = current_contexts; before_sandboxes = current_sandboxes; }
    else { ck_assert(current_contexts == before_contexts); ck_assert(current_sandboxes == before_sandboxes); }
    ck_assert(hosts.stop(id));
    ck_assert(hosts.unbind(id));
  }
  std::filesystem::remove_all(directory);
}
END_TEST

int main() {
  auto* suite = suite_create("Inspection service");
  auto* tc = tcase_create("caller privacy");
  tcase_add_test(tc, test_inspection_filters_order_and_has_no_mutations);
  tcase_add_test(tc, test_inspection_rejects_overrides_and_bad_selectors);
  tcase_add_test(tc, test_inspection_propagates_corruption_and_storage_errors);
  tcase_add_test(tc, test_inspection_host_reopen_and_explicit_binding);
  suite_add_tcase(suite, tc);
  auto* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
