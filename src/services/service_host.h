#pragma once

#include "ceo/task_registry.h"
#include "services/host_records.h"

#include <map>
#include <memory>

namespace iris::service {

struct ServiceHostSnapshot {
  ServiceHostRecord record{};
  ServiceHostState state{ServiceHostState::Stopped};
  std::optional<ceo::TaskID> task_id;
  bool bound{false};
  bool healthy{false};
};

// One manager owns the runtime bindings for a store. Bound handlers must outlive
// their binding; stop and unbind before destroying a handler. Calls are serialized.
class ServiceHostManager {
public:
  ServiceHostManager(referee::SqliteStore& store, ceo::TaskRegistry& tasks);
  ~ServiceHostManager();
  ServiceHostManager(const ServiceHostManager&) = delete;
  ServiceHostManager& operator=(const ServiceHostManager&) = delete;

  referee::Result<void> recover();
  referee::Result<void> create(const ServiceDescriptor& descriptor);
  referee::Result<void> bind(referee::ObjectID id, ServiceObject& handler);
  referee::Result<void> unbind(referee::ObjectID id);
  referee::Result<void> start(referee::ObjectID id);
  referee::Result<void> stop(referee::ObjectID id);
  referee::Result<void> restart(referee::ObjectID id);
  referee::Result<ServiceHostSnapshot> inspect(referee::ObjectID id) const;
  std::vector<ServiceHostSnapshot> list() const;

  const ServiceRegistry& registry() const { return registry_; }

private:
  struct Host;
  referee::Result<Host*> find(referee::ObjectID id);
  referee::SqliteStore& store_;
  ceo::TaskRegistry& tasks_;
  ServiceRegistry registry_;
  std::map<std::string, std::unique_ptr<Host>> hosts_;
  bool recovered_{false};
};

} // namespace iris::service
