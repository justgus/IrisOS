#pragma once

#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <optional>
#include <string>
#include <vector>

namespace iris::service {

struct CapabilityGrant {
  std::string name;
};

struct CapabilityContext {
  referee::ObjectID id{};
  referee::ObjectID subject{};
  std::optional<referee::ObjectID> sandbox;
  std::vector<CapabilityGrant> grants{};
};

struct CapabilityContextRecord {
  referee::ObjectRef ref{};
  CapabilityContext context{};
};

constexpr referee::TypeID kCapabilityContextType{0x5356434341500001ULL};

class CapabilityContextStore {
public:
  explicit CapabilityContextStore(referee::SqliteStore& store);

  referee::Result<CapabilityContextRecord> persist_context(const CapabilityContext& context);
  referee::Result<std::optional<CapabilityContextRecord>> get_context(referee::ObjectID id);
  referee::Result<std::vector<CapabilityContextRecord>> list_contexts();
  referee::Result<std::vector<CapabilityContextRecord>> list_contexts_for_subject(
      referee::ObjectID subject);

private:
  referee::SqliteStore& store_;
};

} // namespace iris::service
