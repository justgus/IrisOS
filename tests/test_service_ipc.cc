extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "referee/referee.h"
#include "services/service.h"

#include <chrono>
#include <string>

using namespace referee;
using namespace iris::service;

namespace {

class EchoService final : public ServiceObject {
public:
  EchoService(ObjectID id, TypeID type, std::string name)
      : ack_type_{0xACCA0001ULL} {
    desc_.id = id;
    desc_.type = type;
    desc_.name = std::move(name);
    Endpoint ep;
    ep.name = "echo";
    ep.type = TypeID{0xE001ULL};
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

} // namespace

START_TEST(test_service_registry_register_resolve_unregister)
{
  ServiceRegistry registry;
  EchoService svc(ObjectID::random(), TypeID{0x9001ULL}, "echo-service");

  ck_assert_msg(registry.register_service(svc.descriptor(), &svc), "register_service failed");

  auto by_name = registry.resolve_by_name("echo-service");
  ck_assert_msg(by_name, "resolve_by_name failed: %s", by_name.error->message.c_str());
  ck_assert_msg(by_name.value->has_value(), "expected service by name");
  ck_assert(by_name.value->value().id == svc.descriptor().id);

  auto by_type = registry.resolve_by_type(svc.descriptor().type);
  ck_assert_msg(by_type, "resolve_by_type failed: %s", by_type.error->message.c_str());
  ck_assert_msg(by_type.value->has_value(), "expected service by type");
  ck_assert(by_type.value->value().id == svc.descriptor().id);

  ck_assert_msg(registry.unregister_service(svc.descriptor().id), "unregister_service failed");

  auto after = registry.resolve_by_name("echo-service");
  ck_assert_msg(after, "resolve_by_name after unregister failed: %s", after.error->message.c_str());
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
  ck_assert_msg(response, "send_request failed: %s", response.error->message.c_str());
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

Suite* service_ipc_suite(void) {
  Suite* s = suite_create("ServiceIPC");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_service_registry_register_resolve_unregister);
  tcase_add_test(tc, test_ipc_send_receive_ack_and_timeout);

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
