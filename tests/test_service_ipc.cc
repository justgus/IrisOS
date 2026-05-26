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

Suite* service_ipc_suite(void) {
  Suite* s = suite_create("ServiceIPC");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_service_registry_register_resolve_unregister);
  tcase_add_test(tc, test_ipc_send_receive_ack_and_timeout);
  tcase_add_test(tc, test_ipc_enforces_descriptor_required_grants);
  tcase_add_test(tc, test_ipc_enforces_endpoint_required_grants_for_declared_endpoint);

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
