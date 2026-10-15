extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "services/allocation_service.h"
#include "services/service_host.h"

#include <cstdlib>
#include <filesystem>
#include <new>
#include <limits>
#include <nlohmann/json.hpp>

using namespace referee;
using namespace iris::service;
using nlohmann::json;

// One-shot allocator fault for the buffer-size boundary, without a production test hook.
static std::size_t fail_allocation_size = 0;
void* operator new(std::size_t size) {
  if (fail_allocation_size != 0 && size == fail_allocation_size) {
    fail_allocation_size = 0;
    throw std::bad_alloc();
  }
  if (void* pointer = std::malloc(size == 0 ? 1 : size)) return pointer;
  throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
MessageEnvelope request(ObjectID service, ObjectID owner, TypeID operation, json payload) {
  auto out = make_request_to_object(owner, service, operation, json::to_cbor(payload));
  out.endpoint = Endpoint{"", operation, {}};
  out.sandbox = ObjectID::random();
  return out;
}

json send(IpcService& ipc, const MessageEnvelope& envelope) {
  auto response = ipc.send_request(envelope, std::chrono::milliseconds(10));
  ck_assert_msg(response, "%s", response.error ? response.error->message.c_str() : "ok");
  ck_assert(response.value->recipient == envelope.sender);
  ck_assert(response.value->correlation_id == envelope.correlation_id);
  ck_assert(response.value->sandbox == envelope.sandbox);
  ck_assert(response.value->message_type == kAllocationResponseType);
  return json::from_cbor(response.value->payload_cbor);
}

void error(IpcService& ipc, const MessageEnvelope& envelope, ErrorCode expected) {
  auto result = ipc.send_request(envelope, std::chrono::milliseconds(10));
  ck_assert(!result);
  ck_assert_msg(result.error->code == expected, "%s", result.error->message.c_str());
}
}

START_TEST(test_owned_allocation_limits_and_release)
{
  auto id = ObjectID::random(), owner = ObjectID::random(), other = ObjectID::random();
  AllocationService service(id, {12, 8, 3});
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  auto first = send(ipc, request(id, owner, kAllocationAllocateType, {{"size", 5}}));
  ck_assert_uint_eq(first["size"].get<unsigned>(), 5);
  ck_assert(first["owner"].get<std::string>() == owner.to_hex());
  auto found = send(ipc, request(id, owner, kAllocationInspectType, {{"id", first["id"]}}));
  ck_assert(found == first);
  error(ipc, request(id, other, kAllocationInspectType, {{"id", first["id"]}}),
        ErrorCode::FailedPrecondition);
  error(ipc, request(id, other, kAllocationReleaseType, {{"id", first["id"]}}),
        ErrorCode::FailedPrecondition);
  error(ipc, request(id, owner, kAllocationAllocateType, {{"size", 4}}),
        ErrorCode::FailedPrecondition);
  auto second = send(ipc, request(id, other, kAllocationAllocateType, {{"size", 7}}));
  ck_assert(first["id"] != second["id"]);
  error(ipc, request(id, ObjectID::random(), kAllocationAllocateType, {{"size", 1}}),
        ErrorCode::FailedPrecondition);
  ck_assert(send(ipc, request(id, owner, kAllocationReleaseType, {{"id", first["id"]}})) == first);
  error(ipc, request(id, owner, kAllocationReleaseType, {{"id", first["id"]}}),
        ErrorCode::NotFound);
  send(ipc, request(id, owner, kAllocationAllocateType, {{"size", 5}}));
}
END_TEST

START_TEST(test_count_and_disabled_limits)
{
  auto id = ObjectID::random(), owner = ObjectID::random();
  AllocationService service(id, {100, 100, 1});
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  auto first = send(ipc, request(id, owner, kAllocationAllocateType, {{"size", 1}}));
  error(ipc, request(id, owner, kAllocationAllocateType, {{"size", 1}}),
        ErrorCode::FailedPrecondition);
  send(ipc, request(id, owner, kAllocationReleaseType, {{"id", first["id"]}}));
  send(ipc, request(id, owner, kAllocationAllocateType, {{"size", 1}}));
  AllocationService disabled(id, {0, 0, 0});
  ck_assert(!disabled.handle_message(request(id, owner, kAllocationAllocateType, {{"size", 1}})));
}
END_TEST

START_TEST(test_invalid_arguments_and_all_selectors_are_atomic)
{
  auto id = ObjectID::random(), owner = ObjectID::random();
  AllocationService service(id, {1, 1, 1});
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  for (const auto& size : std::vector<json>{0, -1, 1.5, 1e100, true, "1", nullptr,
                                         std::numeric_limits<std::uint64_t>::max()}) {
    error(ipc, request(id, owner, kAllocationAllocateType, {{"size", size}}), ErrorCode::InvalidArgument);
  }
  for (const auto& payload : std::vector<json>{json::array(), json::object(),
                         {{"size", 1}, {"subject", owner.to_hex()}}}) {
    error(ipc, request(id, owner, kAllocationAllocateType, payload), ErrorCode::InvalidArgument);
  }
  auto valid = request(id, owner, kAllocationAllocateType, {{"size", 1}});
  auto bad = valid;
  bad.payload_cbor = {0xff};
  error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint.reset();
  error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint = Endpoint{};
  error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint->name = "allocation.release";
  error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.message_type = kAllocationInspectType;
  error(ipc, bad, ErrorCode::InvalidArgument);
  bad = valid; bad.endpoint->type = TypeID{1};
  error(ipc, bad, ErrorCode::InvalidArgument);
  for (const auto& invalid_id : std::vector<json>{1, -1, 1.5, nullptr, "x", std::string(32, 'z')}) {
    error(ipc, request(id, owner, kAllocationInspectType, {{"id", invalid_id}}), ErrorCode::InvalidArgument);
  }
  error(ipc, request(id, owner, kAllocationInspectType, {{"id", ObjectID::random().to_hex()}}),
        ErrorCode::NotFound);
  // Every rejected request above must leave the single available allocation untouched.
  valid.endpoint = Endpoint{"allocation.allocate", std::nullopt, {}};
  send(ipc, valid);
}
END_TEST

START_TEST(test_buffer_allocation_failure_preserves_quotas)
{
  auto id = ObjectID::random(), owner = ObjectID::random();
  AllocationService service(id, {73, 73, 1});
  ServiceRegistry registry;
  ck_assert(registry.register_service(service.descriptor(), &service));
  IpcService ipc(registry);
  auto envelope = request(id, owner, kAllocationAllocateType, {{"size", 73}});
  fail_allocation_size = 73;
  auto failed = service.handle_message(envelope);
  const auto fault_remaining = fail_allocation_size;
  fail_allocation_size = 0;
  ck_assert_uint_eq(fault_remaining, 0);
  ck_assert(!failed);
  ck_assert(failed.error->code == ErrorCode::IoError);
  // A failed allocation cannot consume either the byte quota or the single count slot.
  auto allocated = send(ipc, envelope);
  send(ipc, request(id, owner, kAllocationReleaseType, {{"id", allocated["id"]}}));
  send(ipc, envelope);
}
END_TEST

START_TEST(test_allocation_host_recovery_starts_empty)
{
  const auto directory = std::filesystem::path("allocation-recovery-" + ObjectID::random().to_hex());
  const auto path = (directory / "store").string();
  std::filesystem::create_directory(directory);
  auto id = ObjectID::random(), owner = ObjectID::random();
  std::string allocation_id;
  {
    SqliteStore store({.filename=path});
    ck_assert(store.open());
    iris::ceo::TaskRegistry tasks;
    AllocationService service(id, {8, 8, 1});
    ServiceHostManager hosts(store, tasks);
    ck_assert(hosts.recover());
    ck_assert(hosts.create(service.descriptor()));
    ck_assert(hosts.bind(id, service));
    ck_assert(hosts.start(id));
    IpcService ipc(hosts.registry());
    allocation_id = send(ipc, request(id, owner, kAllocationAllocateType, {{"size", 8}}))["id"];
    ck_assert(hosts.stop(id));
    ck_assert(hosts.unbind(id));
  }
  {
    SqliteStore store({.filename=path});
    ck_assert(store.open());
    iris::ceo::TaskRegistry tasks;
    AllocationService service(id, {8, 8, 1});
    ServiceHostManager hosts(store, tasks);
    ck_assert(hosts.recover());
    ck_assert(hosts.inspect(id).value->state == ServiceHostState::Stopped);
    ck_assert(!hosts.inspect(id).value->bound);
    ck_assert(!hosts.start(id));
    ck_assert(hosts.bind(id, service));
    ck_assert(hosts.start(id));
    IpcService ipc(hosts.registry());
    error(ipc, request(id, owner, kAllocationInspectType, {{"id", allocation_id}}), ErrorCode::NotFound);
    send(ipc, request(id, owner, kAllocationAllocateType, {{"size", 8}}));
    ck_assert(hosts.stop(id));
    ck_assert(hosts.unbind(id));
  }
  std::filesystem::remove_all(directory);
}
END_TEST

int main() {
  auto* suite = suite_create("Core services");
  auto* tc = tcase_create("allocation");
  tcase_add_test(tc, test_owned_allocation_limits_and_release);
  tcase_add_test(tc, test_count_and_disabled_limits);
  tcase_add_test(tc, test_invalid_arguments_and_all_selectors_are_atomic);
  tcase_add_test(tc, test_allocation_host_recovery_starts_empty);
  tcase_add_test(tc, test_buffer_allocation_failure_preserves_quotas);
  suite_add_tcase(suite, tc);
  auto* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
