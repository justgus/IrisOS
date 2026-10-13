#pragma once

#include "services/service.h"
#include "referee_sqlite/sqlite_store.h"

namespace iris::service {

enum class ServiceHostState { Created, Running, Stopped };

std::string_view service_host_state_name(ServiceHostState state);

constexpr referee::TypeID kServiceHostRecordType{0x535643484F530001ULL};

struct ServiceHostRecord {
  referee::ObjectRef ref{};
  ServiceDescriptor descriptor{};
  ServiceHostState state{ServiceHostState::Created};
  std::uint64_t sequence{0};
  std::uint64_t generation{0};
  std::optional<referee::ObjectID> previous_record;
};

namespace detail {

referee::Result<void> validate_service_descriptor(const ServiceDescriptor& descriptor);
referee::Result<std::vector<ServiceHostRecord>> load_service_hosts(referee::SqliteStore& store);
referee::Result<ServiceHostRecord> append_service_host(
    referee::SqliteStore& store, const ServiceDescriptor& descriptor,
    ServiceHostState state, std::uint64_t generation,
    const std::optional<ServiceHostRecord>& previous);

} // namespace detail
} // namespace iris::service
