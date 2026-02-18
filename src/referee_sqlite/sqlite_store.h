#pragma once

#include "referee/referee.h"

#include <sqlite3.h>
#include <string>
#include <vector>

namespace referee {

struct SqliteConfig {
  std::string filename;     // e.g. "referee.db" or ":memory:"
  bool enable_wal{true};
  bool enable_foreign_keys{true};
};

class SqliteStore {
public:
  explicit SqliteStore(SqliteConfig cfg);
  ~SqliteStore();

  SqliteStore(const SqliteStore&) = delete;
  SqliteStore& operator=(const SqliteStore&) = delete;

  Result<void> open();
  Result<void> close();

  // Schema management
  Result<void> ensure_schema();

  // Transactions
  Result<void> begin();
  Result<void> commit();
  Result<void> rollback();

  // Core operations (immutable objects)
  Result<ObjectRecord> create_object(TypeID type, ObjectID definition_id, const Bytes& payload_cbor);
  Result<ObjectRecord> create_object_with_id(ObjectID object_id, TypeID type, ObjectID definition_id,
                                             const Bytes& payload_cbor);
  Result<std::optional<ObjectRecord>> get_object(ObjectRef ref);
  Result<std::optional<ObjectRecord>> get_latest(ObjectID id);
  Result<std::vector<ObjectRecord>> list_by_type(TypeID type);

  // Edge operations
  Result<void> add_edge(ObjectRef from, ObjectRef to, std::string name, std::string role,
                        const Bytes& props_cbor);
  Result<std::vector<EdgeRecord>> edges_from(ObjectRef from,
                                             std::optional<std::string> name_filter = std::nullopt,
                                             std::optional<std::string> role_filter = std::nullopt);
  Result<std::vector<EdgeRecord>> edges_to(ObjectRef to,
                                           std::optional<std::string> name_filter = std::nullopt,
                                           std::optional<std::string> role_filter = std::nullopt);

  // read helpers
  static Bytes col_blob(sqlite3_stmt* st, int col);
  static std::string col_text(sqlite3_stmt* st, int col);

private:
  Result<void> exec(std::string_view sql);
  Result<void> prepare_all();

  // bind helpers
  static void bind_blob(sqlite3_stmt* st, int idx, const std::uint8_t* data, int len);
  static void bind_text(sqlite3_stmt* st, int idx, std::string_view s);

private:
  SqliteConfig cfg_;
  sqlite3* db_{nullptr};

  // Prepared statements
  sqlite3_stmt* st_insert_object_{nullptr};
  sqlite3_stmt* st_get_object_{nullptr};
  sqlite3_stmt* st_get_latest_{nullptr};
  sqlite3_stmt* st_list_by_type_{nullptr};

  sqlite3_stmt* st_insert_edge_{nullptr};
  sqlite3_stmt* st_edges_from_{nullptr};
  sqlite3_stmt* st_edges_from_named_{nullptr};
  sqlite3_stmt* st_edges_from_role_{nullptr};
  sqlite3_stmt* st_edges_from_named_role_{nullptr};
  sqlite3_stmt* st_edges_to_{nullptr};
  sqlite3_stmt* st_edges_to_named_{nullptr};
  sqlite3_stmt* st_edges_to_role_{nullptr};
  sqlite3_stmt* st_edges_to_named_role_{nullptr};
};

} // namespace referee
