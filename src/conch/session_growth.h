#pragma once

#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstddef>
#include <string>

namespace iris::conch {

struct SessionState {
  referee::ObjectRef session{};
  referee::GraphChangeCursor cursor{};
};

struct SessionUpdateResult {
  referee::GraphChangeCursor consumed_cursor{};
  std::size_t changes_examined{0};
  std::size_t conchos_created{0};
  std::size_t conchos_reused{0};
};

referee::Result<SessionState> create_session(iris::refract::SchemaRegistry& registry,
                                             referee::SqliteStore& store,
                                             std::string name,
                                             referee::GraphChangeCursor cursor);

referee::Result<SessionUpdateResult> update_session_from_graph(
    iris::refract::SchemaRegistry& registry,
    referee::SqliteStore& store,
    SessionState& state);

} // namespace iris::conch
