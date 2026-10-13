extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "services/service_host.h"
#include "services/capability_context.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <unistd.h>

using namespace referee;
using namespace iris::service;

namespace {

template <typename T>
const char* message(const Result<T>& result) {
  return result.error ? result.error->message.c_str() : "ok";
}

class EchoService final : public ServiceObject {
public:
  explicit EchoService(ServiceDescriptor descriptor) : descriptor_(std::move(descriptor)) {}
  ServiceDescriptor descriptor() const override { return descriptor_; }
  Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override {
    ++calls;
    return Result<MessageEnvelope>::ok(make_response(request, descriptor_.id, request.message_type,
                                                     request.payload_cbor));
  }
  int calls{0};
private:
  ServiceDescriptor descriptor_;
};

ServiceDescriptor descriptor() {
  return ServiceDescriptor{ObjectID::random(), TypeID{0xE570}, "echo", {"echo.call"},
                           {Endpoint{"echo.send", TypeID{0xE571}, {"echo.send"}}}};
}

MessageEnvelope request(ObjectID id) {
  auto envelope = make_request_to_object(ObjectID::random(), id, TypeID{0xE571},
                                          cbor_from_json_string("{\"ping\":42}"));
  envelope.endpoint = Endpoint{"echo.send", TypeID{0xE571}, {}};
  return envelope;
}

struct TempStore {
  TempStore() {
    char pattern[] = "/tmp/iris-service-host-XXXXXX";
    auto* created = ::mkdtemp(pattern);
    ck_assert_ptr_nonnull(created);
    directory = created;
  }
  ~TempStore() { std::filesystem::remove_all(directory); }
  std::string path() const { return directory + "/registry"; }
  std::string directory;
};

} // namespace

START_TEST(test_host_lifecycle_and_routing)
{
  SqliteStore store(SqliteConfig{.filename=":memory:"});
  ck_assert(store.open());
  iris::ceo::TaskRegistry tasks;
  ServiceHostManager hosts(store, tasks);
  auto desc = descriptor();
  EchoService echo(desc);
  ck_assert(!hosts.create(desc));
  ck_assert(hosts.recover());
  ck_assert(hosts.create(desc));
  auto created = hosts.inspect(desc.id);
  ck_assert(created);
  ck_assert(created.value->state == ServiceHostState::Created);
  ck_assert(created.value->task_id.has_value());
  auto created_task = tasks.get_task(created.value->task_id.value());
  ck_assert(created_task.value->state == iris::ceo::TaskState::Created);
  ck_assert(created_task.value->mode == iris::ceo::TaskMode::Service);
  IpcService ipc(hosts.registry());
  auto envelope = request(desc.id);
  auto stopped_response = ipc.send_request(envelope, std::chrono::milliseconds(5));
  ck_assert(!stopped_response);
  ck_assert(stopped_response.error->code == ErrorCode::FailedPrecondition);
  ck_assert(!hosts.start(desc.id));
  ck_assert(hosts.bind(desc.id, echo));
  ck_assert(hosts.start(desc.id));
  auto running = hosts.inspect(desc.id);
  ck_assert(running.value->healthy);
  auto response = ipc.send_request(envelope, std::chrono::milliseconds(5));
  ck_assert_msg(response, "%s", message(response));
  ck_assert(response.value->payload_cbor == envelope.payload_cbor);
  ck_assert(response.value->correlation_id == envelope.correlation_id);
  ck_assert(!hosts.start(desc.id));
  ck_assert(!hosts.bind(desc.id, echo));
  ck_assert(!hosts.unbind(desc.id));
  ck_assert(hosts.restart(desc.id));
  auto restarted = hosts.inspect(desc.id);
  ck_assert(restarted.value->task_id != running.value->task_id);
  ck_assert_uint_eq(restarted.value->record.generation, 2);
  ck_assert(tasks.get_task(running.value->task_id.value()).value->state == iris::ceo::TaskState::Canceled);
  ck_assert(ipc.send_request(envelope, std::chrono::milliseconds(5)));
  ck_assert_int_eq(echo.calls, 2);
  ck_assert(hosts.stop(desc.id));
  ck_assert(!hosts.stop(desc.id));
  ck_assert(!ipc.send_request(envelope, std::chrono::milliseconds(5)));
  ck_assert(hosts.registry().resolve_by_name(desc.name));
  ck_assert(hosts.unbind(desc.id));
  ck_assert(!hosts.restart(desc.id));
  ck_assert(hosts.bind(desc.id, echo));
  ck_assert(hosts.start(desc.id));
  ck_assert(tasks.kill_task(hosts.inspect(desc.id).value->task_id.value()));
  ck_assert(!hosts.inspect(desc.id).value->healthy);
  ck_assert(!ipc.send_request(envelope, std::chrono::milliseconds(5)));
  ck_assert(hosts.restart(desc.id));
  ck_assert(hosts.inspect(desc.id).value->healthy);
}
END_TEST

START_TEST(test_host_recovery_requires_binding_and_start)
{
  TempStore file;
  auto desc = descriptor();
  ObjectRef created_ref;
  {
    SqliteStore store(SqliteConfig{.filename=file.path()});
    ck_assert(store.open());
    iris::ceo::TaskRegistry tasks;
    EchoService echo(desc);
    ServiceHostManager hosts(store, tasks);
    ck_assert(hosts.recover());
    ck_assert(hosts.create(desc));
    created_ref = hosts.inspect(desc.id).value->record.ref;
    ck_assert(hosts.bind(desc.id, echo));
    ck_assert(hosts.start(desc.id));
    ck_assert(store.close());
  }
  {
    SqliteStore store(SqliteConfig{.filename=file.path()});
    ck_assert(store.open());
    iris::ceo::TaskRegistry tasks;
    // Occupy a reused numeric task ID before recovery; it cannot become a host task.
    ck_assert(tasks.spawn_task(ObjectID::random()));
    EchoService echo(desc);
    ServiceHostManager hosts(store, tasks);
    ck_assert(hosts.recover());
    ck_assert(!hosts.recover());
    auto recovered = hosts.inspect(desc.id);
    ck_assert(recovered.value->state == ServiceHostState::Stopped);
    ck_assert(recovered.value->record.state == ServiceHostState::Running);
    ck_assert(!recovered.value->task_id);
    ck_assert(!recovered.value->bound);
    ck_assert(!recovered.value->healthy);
    ck_assert(hosts.registry().resolve_by_name(desc.name).value.value() == desc);
    ck_assert(hosts.registry().resolve_by_type(desc.type).value.value() == desc);
    ck_assert(store.get_object(created_ref));
    ck_assert(!hosts.start(desc.id));
    ck_assert(hosts.bind(desc.id, echo));
    IpcService ipc(hosts.registry());
    ck_assert(!ipc.send_request(request(desc.id), std::chrono::milliseconds(5)));
    ck_assert(hosts.start(desc.id));
    ck_assert(hosts.inspect(desc.id).value->task_id.value() != 1);
    ck_assert_uint_eq(hosts.inspect(desc.id).value->record.generation, 2);
    ck_assert(ipc.send_request(request(desc.id), std::chrono::milliseconds(5)));
    auto named = request(desc.id);
    named.recipient.reset();
    named.endpoint = Endpoint{desc.name, TypeID{0xE571}, {}};
    ck_assert(ipc.send_request(named, std::chrono::milliseconds(5)));
    auto typed = request(desc.id);
    typed.recipient.reset();
    typed.endpoint = Endpoint{"", desc.type, {}};
    ck_assert(ipc.send_request(typed, std::chrono::milliseconds(5)));
    ck_assert(hosts.stop(desc.id));
    ck_assert(store.close());
  }
  {
    SqliteStore store(SqliteConfig{.filename=file.path()});
    ck_assert(store.open());
    iris::ceo::TaskRegistry tasks;
    ServiceHostManager hosts(store, tasks);
    ck_assert(hosts.recover());
    ck_assert_uint_eq(hosts.inspect(desc.id).value->record.sequence, 4);
    ck_assert(hosts.inspect(desc.id).value->record.state == ServiceHostState::Stopped);
    ck_assert_uint_eq(hosts.list().size(), 1);
  }
}
END_TEST

START_TEST(test_host_rejects_duplicates_bad_binding_and_failed_storage)
{
  SqliteStore store(SqliteConfig{.filename=":memory:"});
  ck_assert(store.open());
  iris::ceo::TaskRegistry tasks;
  ServiceHostManager hosts(store, tasks);
  ck_assert(hosts.recover());
  auto desc = descriptor();
  EchoService echo(desc);
  ck_assert(hosts.create(desc));
  ck_assert(!hosts.create(desc));
  auto duplicate_name = desc;
  duplicate_name.id = ObjectID::random();
  duplicate_name.type = TypeID{0xE580};
  ck_assert(!hosts.create(duplicate_name));
  auto duplicate_type = desc;
  duplicate_type.id = ObjectID::random();
  duplicate_type.name = "other";
  ck_assert(!hosts.create(duplicate_type));
  auto invalid = duplicate_type;
  invalid.type = TypeID{0xE580};
  invalid.endpoints.push_back(invalid.endpoints.front());
  ck_assert(!hosts.create(invalid));
  ck_assert_uint_eq(store.list_by_type(kServiceHostRecordType).value->size(), 1);
  EchoService wrong(duplicate_name);
  ck_assert(!hosts.bind(desc.id, wrong));
  ck_assert(!hosts.bind(ObjectID::random(), echo));
  ck_assert(hosts.bind(desc.id, echo));
  ck_assert(store.close());
  auto failed_start = hosts.start(desc.id);
  ck_assert(!failed_start);
  ck_assert(failed_start.error->code == ErrorCode::FailedPrecondition);
  ck_assert(!hosts.inspect(desc.id).value->healthy);
  ck_assert(hosts.registry().handler_for(desc.id) == nullptr);
  ck_assert(store.open());
  ck_assert(hosts.start(desc.id));
  ck_assert(store.close());
  ck_assert(!hosts.stop(desc.id));
  ck_assert(hosts.inspect(desc.id).value->state == ServiceHostState::Stopped);
  ck_assert(hosts.registry().handler_for(desc.id) == nullptr);
  ck_assert(store.open());
  ck_assert(hosts.start(desc.id));
}
END_TEST

START_TEST(test_host_recovery_rejects_corruption_without_partial_registry)
{
  SqliteStore store(SqliteConfig{.filename=":memory:"});
  ck_assert(store.open());
  auto desc = descriptor();
  auto valid = iris::service::detail::append_service_host(store, desc, ServiceHostState::Created, 0, std::nullopt);
  ck_assert(valid);
  ck_assert(store.create_object(kServiceHostRecordType, {}, {0xff}));
  iris::ceo::TaskRegistry tasks;
  ServiceHostManager hosts(store, tasks);
  auto recovered = hosts.recover();
  ck_assert(!recovered);
  ck_assert(recovered.error->code == ErrorCode::CorruptData);
  ck_assert(hosts.list().empty());
  ck_assert(!hosts.registry().resolve_by_name(desc.name));
}
END_TEST

START_TEST(test_host_recovery_rejects_conflicting_history)
{
  SqliteStore store(SqliteConfig{.filename=":memory:"});
  ck_assert(store.open());
  auto desc = descriptor();
  auto created = iris::service::detail::append_service_host(store, desc, ServiceHostState::Created, 0, std::nullopt);
  ck_assert(created);
  ck_assert(iris::service::detail::append_service_host(store, desc, ServiceHostState::Running, 1, created.value));
  ck_assert(iris::service::detail::append_service_host(store, desc, ServiceHostState::Stopped, 0, created.value));
  iris::ceo::TaskRegistry tasks;
  ServiceHostManager hosts(store, tasks);
  auto recovered = hosts.recover();
  ck_assert(!recovered);
  ck_assert(recovered.error->code == ErrorCode::CorruptData);
}
END_TEST

START_TEST(test_host_preserves_capability_enforcement)
{
  SqliteStore store(SqliteConfig{.filename=":memory:"});
  ck_assert(store.open());
  iris::ceo::TaskRegistry tasks;
  auto desc = descriptor();
  EchoService echo(desc);
  ServiceHostManager hosts(store, tasks);
  ck_assert(hosts.recover());
  ck_assert(hosts.create(desc));
  ck_assert(hosts.bind(desc.id, echo));
  ck_assert(hosts.start(desc.id));
  CapabilityContextStore contexts(store);
  CapabilityContextAuthorizer authorizer(contexts);
  IpcService ipc(hosts.registry(), &authorizer);
  auto envelope = request(desc.id);
  auto denied = ipc.send_request(envelope, std::chrono::milliseconds(5));
  ck_assert(!denied);
  ck_assert_int_eq(echo.calls, 0);
  CapabilityContext context{ObjectID::random(), envelope.sender, std::nullopt,
                             {CapabilityGrant{"echo.call"}, CapabilityGrant{"echo.send"}}};
  ck_assert(contexts.persist_context(context));
  ck_assert(ipc.send_request(envelope, std::chrono::milliseconds(5)));
  ck_assert(hosts.restart(desc.id));
  ck_assert(ipc.send_request(envelope, std::chrono::milliseconds(5)));
  ck_assert_int_eq(echo.calls, 2);
}
END_TEST

int main() {
  Suite* suite = suite_create("ServiceHost");
  TCase* cases = tcase_create("lifecycle");
  tcase_add_test(cases, test_host_lifecycle_and_routing);
  tcase_add_test(cases, test_host_recovery_requires_binding_and_start);
  tcase_add_test(cases, test_host_rejects_duplicates_bad_binding_and_failed_storage);
  tcase_add_test(cases, test_host_recovery_rejects_corruption_without_partial_registry);
  tcase_add_test(cases, test_host_recovery_rejects_conflicting_history);
  tcase_add_test(cases, test_host_preserves_capability_enforcement);
  suite_add_tcase(suite, cases);
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
