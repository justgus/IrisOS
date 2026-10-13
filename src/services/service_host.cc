#include "services/service_host.h"

#include <limits>

namespace iris::service {

struct ServiceHostManager::Host final : ServiceObject {
  explicit Host(ceo::TaskRegistry& registry) : tasks(registry) {}

  ServiceDescriptor descriptor() const override { return record.descriptor; }

  bool healthy() const {
    if (state != ServiceHostState::Running || !task_id || !handler) return false;
    auto task = tasks.get_task(task_id.value());
    return task && task.value->object_id == record.descriptor.id
           && task.value->state == ceo::TaskState::Running
           && task.value->mode == ceo::TaskMode::Service;
  }

  referee::Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override {
    if (!healthy()) {
      return referee::Result<MessageEnvelope>::err(referee::ErrorCode::FailedPrecondition,
                                                   "service host is not healthy");
    }
    return handler->handle_message(request);
  }

  ServiceHostSnapshot snapshot() const {
    return ServiceHostSnapshot{record, state, task_id, handler != nullptr, healthy()};
  }

  ceo::TaskRegistry& tasks;
  ServiceHostRecord record;
  ServiceHostState state{ServiceHostState::Stopped};
  std::optional<ceo::TaskID> task_id;
  ServiceObject* handler{nullptr};
};

ServiceHostManager::ServiceHostManager(referee::SqliteStore& store, ceo::TaskRegistry& tasks)
    : store_(store), tasks_(tasks) {}

ServiceHostManager::~ServiceHostManager() {
  for (const auto& [id, host] : hosts_) {
    (void)id;
    if (host->task_id) (void)tasks_.kill_task(host->task_id.value());
  }
}

referee::Result<void> ServiceHostManager::recover() {
  if (recovered_) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "service registry already recovered");
  }
  auto records = detail::load_service_hosts(store_);
  if (!records) return referee::Result<void>::err(records.error.value());
  ServiceRegistry registry;
  std::map<std::string, std::unique_ptr<Host>> hosts;
  for (const auto& record : records.value.value()) {
    auto registered = registry.register_descriptor(record.descriptor);
    if (!registered) {
      return referee::Result<void>::err(referee::ErrorCode::CorruptData, registered.error->message);
    }
    auto host = std::make_unique<Host>(tasks_);
    host->record = record;
    // Persisted Running is historical evidence, never a process-local task binding.
    hosts.emplace(record.descriptor.id.to_hex(), std::move(host));
  }
  registry_ = std::move(registry);
  hosts_ = std::move(hosts);
  recovered_ = true;
  return referee::Result<void>::ok();
}

referee::Result<ServiceHostManager::Host*> ServiceHostManager::find(referee::ObjectID id) {
  if (!recovered_) {
    return referee::Result<Host*>::err(referee::ErrorCode::FailedPrecondition,
                                        "service registry has not been recovered");
  }
  auto it = hosts_.find(id.to_hex());
  if (it == hosts_.end()) {
    return referee::Result<Host*>::err(referee::ErrorCode::NotFound, "service host not found");
  }
  return referee::Result<Host*>::ok(it->second.get());
}

referee::Result<void> ServiceHostManager::create(const ServiceDescriptor& descriptor) {
  if (!recovered_) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "service registry has not been recovered");
  }
  auto valid = detail::validate_service_descriptor(descriptor);
  if (!valid) return valid;
  // Preflight uniqueness before writing any immutable record.
  auto staged = registry_;
  auto registered = staged.register_descriptor(descriptor);
  if (!registered) return registered;
  auto occupied = store_.get_latest(descriptor.id);
  if (occupied) {
    return referee::Result<void>::err(referee::ErrorCode::AlreadyExists,
                                      "service identity is already a stored object");
  }
  if (occupied.error->code != referee::ErrorCode::NotFound) {
    return referee::Result<void>::err(occupied.error.value());
  }
  auto host = std::make_unique<Host>(tasks_);
  auto task = tasks_.create_task(descriptor.id, std::nullopt, descriptor.name, ceo::TaskMode::Service);
  if (!task) return referee::Result<void>::err(task.error.value());
  auto record = detail::append_service_host(store_, descriptor, ServiceHostState::Created, 0, std::nullopt);
  if (!record) {
    (void)tasks_.kill_task(task.value->id);
    return referee::Result<void>::err(record.error.value());
  }
  host->record = record.value.value();
  host->state = ServiceHostState::Created;
  host->task_id = task.value->id;
  hosts_.emplace(descriptor.id.to_hex(), std::move(host));
  registry_ = std::move(staged);
  return referee::Result<void>::ok();
}

referee::Result<void> ServiceHostManager::bind(referee::ObjectID id, ServiceObject& handler) {
  auto found = find(id);
  if (!found) return referee::Result<void>::err(found.error.value());
  auto& host = **found.value;
  if (host.state == ServiceHostState::Running) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "stop the service host before binding");
  }
  try {
    if (handler.descriptor() != host.record.descriptor) {
      return referee::Result<void>::err(referee::ErrorCode::InvalidArgument,
                                        "handler descriptor does not match persisted service");
    }
  } catch (...) {
    return referee::Result<void>::err(referee::ErrorCode::Internal, "handler descriptor failed");
  }
  host.handler = &handler;
  return referee::Result<void>::ok();
}

referee::Result<void> ServiceHostManager::unbind(referee::ObjectID id) {
  auto found = find(id);
  if (!found) return referee::Result<void>::err(found.error.value());
  auto& host = **found.value;
  if (host.state == ServiceHostState::Running) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "stop the service host before unbinding");
  }
  host.handler = nullptr;
  return referee::Result<void>::ok();
}

referee::Result<void> ServiceHostManager::start(referee::ObjectID id) {
  auto found = find(id);
  if (!found) return referee::Result<void>::err(found.error.value());
  auto& host = **found.value;
  if (host.state == ServiceHostState::Running) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "service host already running");
  }
  if (!host.handler) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "service host has no bound handler");
  }
  if (host.record.generation == std::numeric_limits<std::uint64_t>::max()) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition,
                                      "service host generation exhausted");
  }
  auto task = tasks_.create_task(id, std::nullopt, host.record.descriptor.name, ceo::TaskMode::Service);
  if (!task) return referee::Result<void>::err(task.error.value());
  auto started = tasks_.start_task(task.value->id);
  if (!started) {
    (void)tasks_.kill_task(task.value->id);
    return started;
  }
  auto record = detail::append_service_host(store_, host.record.descriptor, ServiceHostState::Running,
                                            host.record.generation + 1, host.record);
  if (!record) {
    (void)tasks_.kill_task(task.value->id);
    return referee::Result<void>::err(record.error.value());
  }
  if (host.task_id) (void)tasks_.kill_task(host.task_id.value());
  host.task_id = task.value->id;
  host.record = record.value.value();
  host.state = ServiceHostState::Running;
  return registry_.bind_handler(id, &host);
}

referee::Result<void> ServiceHostManager::stop(referee::ObjectID id) {
  auto found = find(id);
  if (!found) return referee::Result<void>::err(found.error.value());
  auto& host = **found.value;
  if (host.state == ServiceHostState::Stopped) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition, "service host already stopped");
  }
  // Disable routing even if the subsequent persistent write fails.
  auto detached = registry_.bind_handler(id, nullptr);
  if (!detached) return detached;
  if (host.task_id) {
    auto task = tasks_.get_task(host.task_id.value());
    if (task && (task.value->state == ceo::TaskState::Created
                 || task.value->state == ceo::TaskState::Running
                 || task.value->state == ceo::TaskState::Waiting)) {
      auto stopped = tasks_.stop_task(host.task_id.value());
      if (!stopped) return stopped;
    }
    task = tasks_.get_task(host.task_id.value());
    if (task && task.value->state == ceo::TaskState::CancelRequested) {
      auto canceled = tasks_.mark_canceled(host.task_id.value());
      if (!canceled) return canceled;
    }
  }
  host.state = ServiceHostState::Stopped;
  auto record = detail::append_service_host(store_, host.record.descriptor, ServiceHostState::Stopped,
                                            host.record.generation, host.record);
  if (!record) return referee::Result<void>::err(record.error.value());
  host.record = record.value.value();
  return referee::Result<void>::ok();
}

referee::Result<void> ServiceHostManager::restart(referee::ObjectID id) {
  auto found = find(id);
  if (!found) return referee::Result<void>::err(found.error.value());
  if (!(**found.value).handler) {
    return referee::Result<void>::err(referee::ErrorCode::FailedPrecondition, "service host has no bound handler");
  }
  if ((**found.value).state != ServiceHostState::Stopped) {
    auto stopped = stop(id);
    if (!stopped) return stopped;
  }
  return start(id);
}

referee::Result<ServiceHostSnapshot> ServiceHostManager::inspect(referee::ObjectID id) const {
  auto it = hosts_.find(id.to_hex());
  if (it == hosts_.end()) {
    return referee::Result<ServiceHostSnapshot>::err(referee::ErrorCode::NotFound, "service host not found");
  }
  return referee::Result<ServiceHostSnapshot>::ok(it->second->snapshot());
}

std::vector<ServiceHostSnapshot> ServiceHostManager::list() const {
  std::vector<ServiceHostSnapshot> out;
  for (const auto& [id, host] : hosts_) {
    (void)id;
    out.push_back(host->snapshot());
  }
  return out;
}

} // namespace iris::service
