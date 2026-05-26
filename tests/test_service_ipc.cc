extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "referee/referee.h"
#include "services/capability_context.h"
#include "services/service.h"

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

using namespace referee;
using namespace iris::service;

namespace {

template <typename T>
const char* result_message(const Result<T>& r) {
  return r.error.has_value() ? r.error->message.c_str() : "ok";
}

class EchoService final : public ServiceObject {
public:
  EchoService(ObjectID id,
              TypeID type,
              std::string name,
              std::vector<std::string> required_grants = {},
              std::vector<std::string> endpoint_required_grants = {})
      : ack_type_{0xACCA0001ULL} {
    desc_.id = id;
    desc_.type = type;
    desc_.name = std::move(name);
    desc_.required_grants = std::move(required_grants);
    Endpoint ep;
    ep.name = "echo";
    ep.type = TypeID{0xE001ULL};
    ep.required_grants = std::move(endpoint_required_grants);
    desc_.endpoints.push_back(ep);
  }

  ServiceDescriptor descriptor() const override { return desc_; }

  Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override {
    auto response = make_response(request, desc_.id, ack_type_, request.payload_cbor);
    return Result<MessageEnvelope>::ok(std::move(response));
  }

  TypeID ack_type() const { return ack_type_; }

private:
  ServiceDescriptor desc_{};
  TypeID ack_type_{};
};

CapabilityContext make_context(ObjectID id, ObjectID subject, std::vector<std::string> grants) {
  CapabilityContext context;
  context.id = id;
  context.subject = subject;
  for (auto& grant : grants) {
    context.grants.push_back(CapabilityGrant{std::move(grant)});
  }
  return context;
}

} // namespace

START_TEST(test_service_registry_register_resolve_unregister)
{
  ServiceRegistry registry;
  EchoService svc(ObjectID::random(), TypeID{0x9001ULL}, "echo-service");

  ck_assert_msg(registry.register_service(svc.descriptor(), &svc), "register_service failed");

  auto by_name = registry.resolve_by_name("echo-service");
  ck_assert_msg(by_name, "resolve_by_name failed: %s", result_message(by_name));
  ck_assert_msg(by_name.value->has_value(), "expected service by name");
  ck_assert(by_name.value->value().id == svc.descriptor().id);

  auto by_type = registry.resolve_by_type(svc.descriptor().type);
  ck_assert_msg(by_type, "resolve_by_type failed: %s", result_message(by_type));
  ck_assert_msg(by_type.value->has_value(), "expected service by type");
  ck_assert(by_type.value->value().id == svc.descriptor().id);

  ck_assert_msg(registry.unregister_service(svc.descriptor().id), "unregister_service failed");

  auto after = registry.resolve_by_name("echo-service");
  ck_assert_msg(after, "resolve_by_name after unregister failed: %s", result_message(after));
  ck_assert_msg(!after.value->has_value(), "expected no service after unregister");
}
END_TEST

START_TEST(test_ipc_send_receive_ack_and_timeout)
{
  ServiceRegistry registry;
  EchoService svc(ObjectID::random(), TypeID{0x9002ULL}, "echo-service");
  ck_assert_msg(registry.register_service(svc.descriptor(), &svc), "register_service failed");

  IpcService ipc(registry);

  Endpoint endpoint;
  endpoint.name = "echo-service";

  auto payload = cbor_from_json_string(R"({"ping":"pong"})");
  auto request = make_request_to_endpoint(ObjectID::random(), endpoint, TypeID{0xBEEF0001ULL}, payload);

  auto response = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(response, "send_request failed: %s", result_message(response));
  ck_assert_msg(response.value.has_value(), "expected response envelope");

  const auto& env = response.value.value();
  ck_assert(env.correlation_id == request.correlation_id);
  ck_assert(env.sender == svc.descriptor().id);
  ck_assert_uint_eq(env.message_type.v, svc.ack_type().v);

  auto timeoutR = ipc.send_request(request, std::chrono::milliseconds(0));
  ck_assert_msg(!timeoutR, "expected timeout error");
  ck_assert_msg(timeoutR.error.has_value(), "expected timeout error details");
}
END_TEST

START_TEST(test_ipc_preserves_sandbox_identity_hook)
{
  ServiceRegistry registry;
  EchoService svc(ObjectID::random(), TypeID{0x9005ULL}, "sandbox-service");
  ck_assert_msg(registry.register_service(svc.descriptor(), &svc), "register_service failed");

  IpcService ipc(registry);
  Endpoint endpoint;
  endpoint.name = "sandbox-service";

  auto sandbox_id = ObjectID::random();
  auto request = make_request_to_endpoint(ObjectID::random(), endpoint, TypeID{0xBEEF0004ULL}, {});
  request.sandbox = sandbox_id;

  auto response = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(response, "send_request failed: %s", result_message(response));
  ck_assert_msg(response.value.has_value(), "expected response envelope");
  ck_assert_msg(response.value->sandbox.has_value(), "expected sandbox identity on response");
  ck_assert(response.value->sandbox.value() == sandbox_id);
}
END_TEST

START_TEST(test_ipc_enforces_descriptor_required_grants)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  CapabilityContextStore contexts(store);
  CapabilityContextAuthorizer authorizer(contexts);

  ServiceRegistry registry;
  EchoService svc(ObjectID::random(), TypeID{0x9003ULL}, "restricted-service",
                  {"service.echo.call"});
  ck_assert_msg(registry.register_service(svc.descriptor(), &svc), "register_service failed");

  IpcService ipc(registry, &authorizer);
  Endpoint endpoint;
  endpoint.name = "restricted-service";

  auto sender = ObjectID::random();
  auto request = make_request_to_endpoint(sender, endpoint, TypeID{0xBEEF0002ULL}, {});

  auto denied = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(!denied, "expected missing capability grant to fail");
  ck_assert_msg(denied.error.has_value(), "expected error details");
  ck_assert_int_eq((int)denied.error->code, (int)ErrorCode::FailedPrecondition);

  auto context = make_context(ObjectID::random(), sender, {"service.echo.call"});
  auto savedR = contexts.persist_context(context);
  ck_assert_msg(savedR, "persist_context failed: %s", result_message(savedR));

  auto allowed = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(allowed, "send_request with capability failed: %s", result_message(allowed));
  ck_assert(allowed.value->correlation_id == request.correlation_id);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_ipc_enforces_endpoint_required_grants_for_declared_endpoint)
{
  SqliteStore store(SqliteConfig{ .filename=":memory:", .enable_wal=false });
  ck_assert_msg(store.open(), "open failed");
  ck_assert_msg(store.ensure_schema(), "ensure_schema failed");

  CapabilityContextStore contexts(store);
  CapabilityContextAuthorizer authorizer(contexts);

  ServiceRegistry registry;
  EchoService svc(ObjectID::random(), TypeID{0x9004ULL}, "endpoint-service", {},
                  {"service.echo.endpoint"});
  ck_assert_msg(registry.register_service(svc.descriptor(), &svc), "register_service failed");

  IpcService ipc(registry, &authorizer);
  Endpoint endpoint;
  endpoint.name = "echo";

  auto sender = ObjectID::random();
  auto request = make_request_to_object(sender, svc.descriptor().id, TypeID{0xBEEF0003ULL}, {});
  request.endpoint = endpoint;

  auto denied = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(!denied, "expected missing endpoint capability grant to fail");
  ck_assert_msg(denied.error.has_value(), "expected error details");
  ck_assert_int_eq((int)denied.error->code, (int)ErrorCode::FailedPrecondition);

  auto context = make_context(ObjectID::random(), sender, {"service.echo.endpoint"});
  auto savedR = contexts.persist_context(context);
  ck_assert_msg(savedR, "persist_context failed: %s", result_message(savedR));

  auto allowed = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(allowed, "send_request with endpoint capability failed: %s",
                result_message(allowed));
  ck_assert(allowed.value->correlation_id == request.correlation_id);

  ck_assert_msg(store.close(), "close failed");
}
END_TEST

START_TEST(test_memory_service_registers_and_lists_regions)
{
  ServiceRegistry registry;
  MemoryService memory(ObjectID::random());
  ck_assert_msg(registry.register_service(memory.descriptor(), &memory), "register_service failed");

  IpcService ipc(registry);
  auto region_id = ObjectID::random();

  nlohmann::json payload;
  payload["id"] = region_id.to_hex();
  payload["name"] = "main-ram";
  payload["kind"] = "ram";
  payload["base"] = 4096ULL;
  payload["size"] = 8192ULL;

  auto request = make_request_to_object(ObjectID::random(),
                                        memory.descriptor().id,
                                        kMemoryRegisterRegionType,
                                        nlohmann::json::to_cbor(payload));
  request.endpoint = Endpoint{"memory.register_region", kMemoryRegisterRegionType, {}};

  auto response = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(response, "register region failed: %s", result_message(response));
  ck_assert_uint_eq(response.value->message_type.v, kMemoryRegionResponseType.v);

  auto registered = nlohmann::json::from_cbor(response.value->payload_cbor);
  ck_assert_str_eq(registered.at("name").get<std::string>().c_str(), "main-ram");
  ck_assert_str_eq(registered.at("kind").get<std::string>().c_str(), "ram");
  ck_assert(registered.at("writable").get<bool>());
  ck_assert(!registered.at("persistent").get<bool>());

  auto list_request = make_request_to_object(ObjectID::random(),
                                             memory.descriptor().id,
                                             kMemoryListRegionsType,
                                             {});
  list_request.endpoint = Endpoint{"memory.list_regions", kMemoryListRegionsType, {}};

  auto list_response = ipc.send_request(list_request, std::chrono::milliseconds(5));
  ck_assert_msg(list_response, "list regions failed: %s", result_message(list_response));
  ck_assert_uint_eq(list_response.value->message_type.v, kMemoryRegionListResponseType.v);

  auto list_payload = nlohmann::json::from_cbor(list_response.value->payload_cbor);
  ck_assert_uint_eq(list_payload.at("regions").size(), 1);
  ck_assert_str_eq(list_payload.at("regions").at(0).at("id").get<std::string>().c_str(),
                   region_id.to_hex().c_str());
}
END_TEST

START_TEST(test_memory_service_rejects_duplicates_and_invalid_regions)
{
  MemoryService memory(ObjectID::random());
  MemoryRegion ram;
  ram.id = ObjectID::random();
  ram.name = "main-ram";
  ram.kind = MemoryRegionKind::Ram;
  ram.base = 0;
  ram.size = 4096;

  auto registered = memory.register_region(ram);
  ck_assert_msg(registered, "register_region failed: %s", result_message(registered));

  auto duplicate_id = memory.register_region(ram);
  ck_assert_msg(!duplicate_id, "expected duplicate id rejection");
  ck_assert_int_eq((int)duplicate_id.error->code, (int)ErrorCode::AlreadyExists);

  MemoryRegion duplicate_name = ram;
  duplicate_name.id = ObjectID::random();
  auto duplicate_nameR = memory.register_region(duplicate_name);
  ck_assert_msg(!duplicate_nameR, "expected duplicate name rejection");
  ck_assert_int_eq((int)duplicate_nameR.error->code, (int)ErrorCode::AlreadyExists);

  MemoryRegion empty_name = ram;
  empty_name.id = ObjectID::random();
  empty_name.name.clear();
  auto empty_nameR = memory.register_region(empty_name);
  ck_assert_msg(!empty_nameR, "expected empty name rejection");
  ck_assert_int_eq((int)empty_nameR.error->code, (int)ErrorCode::InvalidArgument);

  MemoryRegion empty_region = ram;
  empty_region.id = ObjectID::random();
  empty_region.name = "empty";
  empty_region.size = 0;
  auto empty_regionR = memory.register_region(empty_region);
  ck_assert_msg(!empty_regionR, "expected zero-size rejection");
  ck_assert_int_eq((int)empty_regionR.error->code, (int)ErrorCode::InvalidArgument);
}
END_TEST

START_TEST(test_memory_service_lookup_and_mutability_classification)
{
  ServiceRegistry registry;
  MemoryService memory(ObjectID::random());
  ck_assert_msg(registry.register_service(memory.descriptor(), &memory), "register_service failed");

  MemoryRegion flash;
  flash.id = ObjectID::random();
  flash.name = "boot-flash";
  flash.kind = MemoryRegionKind::Flash;
  flash.base = 65536;
  flash.size = 4096;
  ck_assert_msg(memory.register_region(flash), "flash register failed");

  MemoryRegion readonly;
  readonly.id = ObjectID::random();
  readonly.name = "rom";
  readonly.kind = MemoryRegionKind::ReadOnly;
  readonly.base = 131072;
  readonly.size = 4096;
  ck_assert_msg(memory.register_region(readonly), "readonly register failed");

  ck_assert(memory_region_is_writable(MemoryRegionKind::Flash));
  ck_assert(memory_region_is_persistent(MemoryRegionKind::Flash));
  ck_assert(!memory_region_is_writable(MemoryRegionKind::ReadOnly));
  ck_assert(memory_region_is_persistent(MemoryRegionKind::ReadOnly));

  IpcService ipc(registry);
  nlohmann::json payload;
  payload["id"] = readonly.id.to_hex();

  auto request = make_request_to_object(ObjectID::random(),
                                        memory.descriptor().id,
                                        kMemoryLookupRegionType,
                                        nlohmann::json::to_cbor(payload));
  request.endpoint = Endpoint{"memory.lookup_region", kMemoryLookupRegionType, {}};

  auto response = ipc.send_request(request, std::chrono::milliseconds(5));
  ck_assert_msg(response, "lookup region failed: %s", result_message(response));

  auto lookup_payload = nlohmann::json::from_cbor(response.value->payload_cbor);
  ck_assert(lookup_payload.at("found").get<bool>());
  ck_assert_str_eq(lookup_payload.at("region").at("kind").get<std::string>().c_str(), "read_only");
  ck_assert(!lookup_payload.at("region").at("writable").get<bool>());
  ck_assert(lookup_payload.at("region").at("persistent").get<bool>());
}
END_TEST

Suite* service_ipc_suite(void) {
  Suite* s = suite_create("ServiceIPC");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_service_registry_register_resolve_unregister);
  tcase_add_test(tc, test_ipc_send_receive_ack_and_timeout);
  tcase_add_test(tc, test_ipc_preserves_sandbox_identity_hook);
  tcase_add_test(tc, test_ipc_enforces_descriptor_required_grants);
  tcase_add_test(tc, test_ipc_enforces_endpoint_required_grants_for_declared_endpoint);
  tcase_add_test(tc, test_memory_service_registers_and_lists_regions);
  tcase_add_test(tc, test_memory_service_rejects_duplicates_and_invalid_regions);
  tcase_add_test(tc, test_memory_service_lookup_and_mutability_classification);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = service_ipc_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
