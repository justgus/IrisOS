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

struct SandboxIdentity {
  referee::ObjectID id{};
  std::string name;
  std::vector<referee::ObjectID> subjects{};
};

struct SandboxIdentityRecord {
  referee::ObjectRef ref{};
  SandboxIdentity sandbox{};
};

constexpr referee::TypeID kCapabilityContextType{0x5356434341500001ULL};
constexpr referee::TypeID kSandboxIdentityType{0x53564353414E0001ULL};

class CapabilityContextStore {
public:
  explicit CapabilityContextStore(referee::SqliteStore& store);

  referee::Result<SandboxIdentityRecord> persist_sandbox(const SandboxIdentity& sandbox);
  referee::Result<std::optional<SandboxIdentityRecord>> get_sandbox(referee::ObjectID id);
  referee::Result<std::vector<SandboxIdentityRecord>> list_sandboxes();
  referee::Result<std::vector<SandboxIdentityRecord>> list_sandboxes_for_subject(
      referee::ObjectID subject);

  referee::Result<CapabilityContextRecord> persist_context(const CapabilityContext& context);
  referee::Result<std::optional<CapabilityContextRecord>> get_context(referee::ObjectID id);
  referee::Result<std::vector<CapabilityContextRecord>> list_contexts();
  referee::Result<std::vector<CapabilityContextRecord>> list_contexts_for_subject(
      referee::ObjectID subject);

private:
  referee::SqliteStore& store_;
};

} // namespace iris::service
