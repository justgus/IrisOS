#include "referee_sqlite/sqlite_store.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace referee {

static std::string sqlite_err(sqlite3* db, const char* prefix) {
  std::ostringstream os;
  os << prefix << ": " << (db ? sqlite3_errmsg(db) : "no db");
  return os.str();
}

SqliteStore::SqliteStore(SqliteConfig cfg)
  : cfg_(std::move(cfg)),
    db_(nullptr),
    st_insert_object_(nullptr),
    st_get_object_(nullptr),
    st_get_latest_(nullptr),
    st_list_by_type_(nullptr),
    st_insert_edge_(nullptr),
    st_edges_from_(nullptr),
    st_edges_from_named_(nullptr),
    st_edges_from_role_(nullptr),
    st_edges_from_named_role_(nullptr),
    st_edges_to_(nullptr),
    st_edges_to_named_(nullptr),
    st_edges_to_role_(nullptr),
    st_edges_to_named_role_(nullptr) {}
SqliteStore::~SqliteStore() { (void)close(); }

//--------
Result<void> SqliteStore::open() {
  if (db_) return Result<void>::ok();

  if (sqlite3_open(cfg_.filename.c_str(), &db_) != SQLITE_OK) {
    auto msg = sqlite_err(db_, "sqlite3_open failed");
    sqlite3_close(db_);
    db_ = nullptr;
    return Result<void>::err(msg);
  }

  if (cfg_.enable_foreign_keys) {
    auto r = exec("PRAGMA foreign_keys=ON;");
    if (!r) return r;
  }
  if (cfg_.enable_wal) {
    auto r = exec("PRAGMA journal_mode=WAL;");
    if (!r) return r;
    r = exec("PRAGMA synchronous=NORMAL;");
    if (!r) return r;
  }

  // IMPORTANT: do NOT prepare statements here; schema may not exist yet.
  return Result<void>::ok();
}
//---------

Result<void> SqliteStore::close() {
  if (!db_) return Result<void>::ok();

  // Finalize via SQLite's internal list to avoid stale/invalid stmt pointers.
  for (sqlite3_stmt* st = sqlite3_next_stmt(db_, nullptr);
       st != nullptr;
       st = sqlite3_next_stmt(db_, st)) {
    sqlite3_finalize(st);
  }

  st_insert_object_ = nullptr;
  st_get_object_ = nullptr;
  st_get_latest_ = nullptr;
  st_list_by_type_ = nullptr;

  st_insert_edge_ = nullptr;
  st_edges_from_ = nullptr;
  st_edges_from_named_ = nullptr;
  st_edges_from_role_ = nullptr;
  st_edges_from_named_role_ = nullptr;
  st_edges_to_ = nullptr;
  st_edges_to_named_ = nullptr;
  st_edges_to_role_ = nullptr;
  st_edges_to_named_role_ = nullptr;

  sqlite3_close(db_);
  db_ = nullptr;
  return Result<void>::ok();
}

Result<void> SqliteStore::exec(std::string_view sql) {
  char* err = nullptr;
  int rc = sqlite3_exec(db_, std::string(sql).c_str(), nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    std::string msg = err ? err : "sqlite3_exec failed";
    sqlite3_free(err);
    return Result<void>::err(msg);
  }
  return Result<void>::ok();
}

Result<void> SqliteStore::ensure_schema() {
  // Immutable objects: (object_id, version) primary key.
  // Latest is max(version) for given object_id.
  const char* schema_sql = R"SQL(
    CREATE TABLE IF NOT EXISTS objects (
      object_id   BLOB NOT NULL,
      version     INTEGER NOT NULL,
      type_id     INTEGER NOT NULL,
      definition_id BLOB NOT NULL,
      payload_cbor BLOB NOT NULL,
      created_at_ms INTEGER NOT NULL,
      PRIMARY KEY (object_id, version)
    );

    CREATE INDEX IF NOT EXISTS idx_objects_type ON objects(type_id);
    CREATE INDEX IF NOT EXISTS idx_objects_definition ON objects(definition_id);
    CREATE INDEX IF NOT EXISTS idx_objects_oid ON objects(object_id);

    CREATE TABLE IF NOT EXISTS edges (
      from_id     BLOB NOT NULL,
      from_ver    INTEGER NOT NULL,
      to_id       BLOB NOT NULL,
      to_ver      INTEGER NOT NULL,
      name        TEXT NOT NULL,
      role        TEXT NOT NULL,
      props_cbor  BLOB NOT NULL,
      created_at_ms INTEGER NOT NULL
    );

    CREATE INDEX IF NOT EXISTS idx_edges_from ON edges(from_id, from_ver, name, role);
    CREATE INDEX IF NOT EXISTS idx_edges_to   ON edges(to_id, to_ver, name, role);
  )SQL";

//--------
  auto r = exec(schema_sql);
  if (!r) return r;

  // Prepare statements now that schema exists
  if (!st_insert_object_) {
    auto p = prepare_all();
    if (!p) return p;
  }

  return Result<void>::ok();
}
//---------


Result<void> SqliteStore::prepare_all() {
  auto prep = [&](sqlite3_stmt** out, const char* sql) -> Result<void> {
    if (sqlite3_prepare_v2(db_, sql, -1, out, nullptr) != SQLITE_OK) {
      return Result<void>::err(sqlite_err(db_, "sqlite3_prepare_v2 failed"));
    }
    return Result<void>::ok();
  };

  // objects
  {
    auto r = prep(&st_insert_object_,
      "INSERT INTO objects(object_id, version, type_id, definition_id, payload_cbor, created_at_ms) "
      "VALUES(?,?,?,?,?,?);");
    if (!r) return r;

    r = prep(&st_get_object_,
      "SELECT object_id, version, type_id, definition_id, payload_cbor, created_at_ms "
      "FROM objects WHERE object_id=? AND version=? LIMIT 1;");
    if (!r) return r;

    r = prep(&st_get_latest_,
      "SELECT object_id, version, type_id, definition_id, payload_cbor, created_at_ms "
      "FROM objects WHERE object_id=? ORDER BY version DESC LIMIT 1;");
    if (!r) return r;

    r = prep(&st_list_by_type_,
      "SELECT object_id, version, type_id, definition_id, payload_cbor, created_at_ms "
      "FROM objects WHERE type_id=? ORDER BY created_at_ms ASC;");
    if (!r) return r;
  }

  // edges
  {
    auto r = prep(&st_insert_edge_,
      "INSERT INTO edges(from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms) "
      "VALUES(?,?,?,?,?,?,?,?);");
    if (!r) return r;

    r = prep(&st_edges_from_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE from_id=? AND from_ver=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_from_named_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE from_id=? AND from_ver=? AND name=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_from_role_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE from_id=? AND from_ver=? AND role=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_from_named_role_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE from_id=? AND from_ver=? AND name=? AND role=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_to_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE to_id=? AND to_ver=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_to_named_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE to_id=? AND to_ver=? AND name=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_to_role_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE to_id=? AND to_ver=? AND role=? ORDER BY created_at_ms ASC;");
    if (!r) return r;

    r = prep(&st_edges_to_named_role_,
      "SELECT from_id, from_ver, to_id, to_ver, name, role, props_cbor, created_at_ms "
      "FROM edges WHERE to_id=? AND to_ver=? AND name=? AND role=? ORDER BY created_at_ms ASC;");
    if (!r) return r;
  }

  return Result<void>::ok();
}

void SqliteStore::bind_blob(sqlite3_stmt* st, int idx, const std::uint8_t* data, int len) {
  if (len <= 0) {
    sqlite3_bind_blob(st, idx, "", 0, SQLITE_TRANSIENT);
    return;
  }
  sqlite3_bind_blob(st, idx, data, len, SQLITE_TRANSIENT);
}

void SqliteStore::bind_text(sqlite3_stmt* st, int idx, std::string_view s) {
  sqlite3_bind_text(st, idx, s.data(), (int)s.size(), SQLITE_TRANSIENT);
}

Bytes SqliteStore::col_blob(sqlite3_stmt* st, int col) {
  const auto* p = (const std::uint8_t*)sqlite3_column_blob(st, col);
  int n = sqlite3_column_bytes(st, col);
  if (!p || n <= 0) return {};
  return Bytes(p, p + n);
}

std::string SqliteStore::col_text(sqlite3_stmt* st, int col) {
  const unsigned char* p = sqlite3_column_text(st, col);
  if (!p) return {};
  return std::string((const char*)p);
}

// -----------------------------
// Transactions
// -----------------------------
Result<void> SqliteStore::begin()    { return exec("BEGIN IMMEDIATE;"); }
Result<void> SqliteStore::commit()   { return exec("COMMIT;"); }
Result<void> SqliteStore::rollback() { return exec("ROLLBACK;"); }

// -----------------------------
// Core operations
// -----------------------------
Result<ObjectRecord> SqliteStore::create_object(TypeID type, ObjectID definition_id,
                                                const Bytes& payload_cbor) {
  return create_object_with_id(ObjectID::random(), type, definition_id, payload_cbor);
}

Result<ObjectRecord> SqliteStore::create_object_with_id(ObjectID object_id, TypeID type,
                                                        ObjectID definition_id,
                                                        const Bytes& payload_cbor) {
  if (!db_) return Result<ObjectRecord>::err("db not open");
  if (!st_insert_object_) return Result<ObjectRecord>::err("insert statement not prepared");

  ObjectRecord rec;
  rec.ref.id = object_id;
  rec.ref.ver = Version{1};
  rec.type = type;
  rec.definition_id = definition_id;
  rec.payload_cbor = payload_cbor;
  rec.created_at_unix_ms = unix_ms_now();

  sqlite3_reset(st_insert_object_);
  sqlite3_clear_bindings(st_insert_object_);

  bind_blob(st_insert_object_, 1, rec.ref.id.bytes.data(), 16);
  sqlite3_bind_int64(st_insert_object_, 2, (sqlite3_int64)rec.ref.ver.v);
  sqlite3_bind_int64(st_insert_object_, 3, (sqlite3_int64)rec.type.v);
  bind_blob(st_insert_object_, 4, rec.definition_id.bytes.data(), 16);
  bind_blob(st_insert_object_, 5, rec.payload_cbor.data(), (int)rec.payload_cbor.size());
  sqlite3_bind_int64(st_insert_object_, 6, (sqlite3_int64)rec.created_at_unix_ms);

  int rc = sqlite3_step(st_insert_object_);
  if (rc != SQLITE_DONE) {
    sqlite3_reset(st_insert_object_);
    sqlite3_clear_bindings(st_insert_object_);
    return Result<ObjectRecord>::err(sqlite_err(db_, "insert object failed"));
  }
  sqlite3_reset(st_insert_object_);
  sqlite3_clear_bindings(st_insert_object_);
  return Result<ObjectRecord>::ok(std::move(rec));
}

Result<std::optional<ObjectRecord>> SqliteStore::get_object(ObjectRef ref) {
  if (!db_) return Result<std::optional<ObjectRecord>>::err("db not open");
  if (!st_get_object_) return Result<std::optional<ObjectRecord>>::err("get_object statement not prepared");

  sqlite3_reset(st_get_object_);
  sqlite3_clear_bindings(st_get_object_);

  bind_blob(st_get_object_, 1, ref.id.bytes.data(), 16);
  sqlite3_bind_int64(st_get_object_, 2, (sqlite3_int64)ref.ver.v);

  int rc = sqlite3_step(st_get_object_);
  if (rc == SQLITE_ROW) {
    ObjectRecord rec;
    // More robust: read BLOB(16) directly
    {
      auto oid = col_blob(st_get_object_, 0);
      if (oid.size() == 16) std::memcpy(rec.ref.id.bytes.data(), oid.data(), 16);
      else return Result<std::optional<ObjectRecord>>::err("object_id blob was not 16 bytes");      
    }
    rec.ref.ver = Version{ (std::uint64_t)sqlite3_column_int64(st_get_object_, 1) };
    rec.type = TypeID{ (std::uint64_t)sqlite3_column_int64(st_get_object_, 2) };
    {
      auto def_id = col_blob(st_get_object_, 3);
      if (def_id.size() == 16) std::memcpy(rec.definition_id.bytes.data(), def_id.data(), 16);
      else return Result<std::optional<ObjectRecord>>::err("definition_id blob was not 16 bytes");
    }
    rec.payload_cbor = col_blob(st_get_object_, 4);
    rec.created_at_unix_ms = (std::uint64_t)sqlite3_column_int64(st_get_object_, 5);

    sqlite3_reset(st_get_object_);
    sqlite3_clear_bindings(st_get_object_);
    return Result<std::optional<ObjectRecord>>::ok(std::optional<ObjectRecord>(std::move(rec)));
  }
  if (rc == SQLITE_DONE) {
    sqlite3_reset(st_get_object_);
    sqlite3_clear_bindings(st_get_object_);
    return Result<std::optional<ObjectRecord>>::ok(std::nullopt);
  }
  sqlite3_reset(st_get_object_);
  sqlite3_clear_bindings(st_get_object_);
  return Result<std::optional<ObjectRecord>>::err(sqlite_err(db_, "get_object failed"));
}

Result<std::optional<ObjectRecord>> SqliteStore::get_latest(ObjectID id) {
  if (!db_) return Result<std::optional<ObjectRecord>>::err("db not open");
  if (!st_get_latest_) return Result<std::optional<ObjectRecord>>::err("get_latest statement not prepared");

  sqlite3_reset(st_get_latest_);
  sqlite3_clear_bindings(st_get_latest_);

  bind_blob(st_get_latest_, 1, id.bytes.data(), 16);

  int rc = sqlite3_step(st_get_latest_);
  if (rc == SQLITE_ROW) {
    ObjectRecord rec;
    {
      auto oid = col_blob(st_get_latest_, 0);
      if (oid.size() == 16) std::memcpy(rec.ref.id.bytes.data(), oid.data(), 16);
    }
    rec.ref.ver = Version{ (std::uint64_t)sqlite3_column_int64(st_get_latest_, 1) };
    rec.type = TypeID{ (std::uint64_t)sqlite3_column_int64(st_get_latest_, 2) };
    {
      auto def_id = col_blob(st_get_latest_, 3);
      if (def_id.size() == 16) std::memcpy(rec.definition_id.bytes.data(), def_id.data(), 16);
    }
    rec.payload_cbor = col_blob(st_get_latest_, 4);
    rec.created_at_unix_ms = (std::uint64_t)sqlite3_column_int64(st_get_latest_, 5);
    sqlite3_reset(st_get_latest_);
    sqlite3_clear_bindings(st_get_latest_);
    return Result<std::optional<ObjectRecord>>::ok(std::optional<ObjectRecord>(std::move(rec)));
  }
  if (rc == SQLITE_DONE) {
    sqlite3_reset(st_get_latest_);
    sqlite3_clear_bindings(st_get_latest_);
    return Result<std::optional<ObjectRecord>>::ok(std::nullopt);
  }
  sqlite3_reset(st_get_latest_);
  sqlite3_clear_bindings(st_get_latest_);
  return Result<std::optional<ObjectRecord>>::err(sqlite_err(db_, "get_latest failed"));
}

Result<std::vector<ObjectRecord>> SqliteStore::list_by_type(TypeID type) {
  if (!db_) return Result<std::vector<ObjectRecord>>::err("db not open");
  if (!st_list_by_type_) return Result<std::vector<ObjectRecord>>::err("list_by_type statement not prepared");

  sqlite3_reset(st_list_by_type_);
  sqlite3_clear_bindings(st_list_by_type_);

  sqlite3_bind_int64(st_list_by_type_, 1, (sqlite3_int64)type.v);

  std::vector<ObjectRecord> out;
  for (;;) {
    int rc = sqlite3_step(st_list_by_type_);
    if (rc == SQLITE_DONE) break;
    if (rc != SQLITE_ROW) {
      sqlite3_reset(st_list_by_type_);
      sqlite3_clear_bindings(st_list_by_type_);
      return Result<std::vector<ObjectRecord>>::err(sqlite_err(db_, "list_by_type failed"));
    }

    auto id_bytes = col_blob(st_list_by_type_, 0);
    if (id_bytes.size() != 16) {
      return Result<std::vector<ObjectRecord>>::err("invalid object_id size");
    }

    ObjectRecord rec;
    std::copy(id_bytes.begin(), id_bytes.end(), rec.ref.id.bytes.begin());
    rec.ref.ver = Version{(std::uint64_t)sqlite3_column_int64(st_list_by_type_, 1)};
    rec.type = TypeID{(std::uint64_t)sqlite3_column_int64(st_list_by_type_, 2)};
    {
      auto def_id = col_blob(st_list_by_type_, 3);
      if (def_id.size() == 16) std::copy(def_id.begin(), def_id.end(), rec.definition_id.bytes.begin());
    }
    rec.payload_cbor = col_blob(st_list_by_type_, 4);
    rec.created_at_unix_ms = (std::uint64_t)sqlite3_column_int64(st_list_by_type_, 5);

    out.push_back(std::move(rec));
  }

  sqlite3_reset(st_list_by_type_);
  sqlite3_clear_bindings(st_list_by_type_);
  return Result<std::vector<ObjectRecord>>::ok(std::move(out));
}

// -----------------------------
// Edges
// -----------------------------
Result<void> SqliteStore::add_edge(ObjectRef from, ObjectRef to, std::string name, std::string role,
                                   const Bytes& props_cbor) {
  if (!db_) return Result<void>::err("db not open");
  if (!st_insert_edge_) return Result<void>::err("insert_edge statement not prepared");

  sqlite3_reset(st_insert_edge_);
  sqlite3_clear_bindings(st_insert_edge_);

  bind_blob(st_insert_edge_, 1, from.id.bytes.data(), 16);
  sqlite3_bind_int64(st_insert_edge_, 2, (sqlite3_int64)from.ver.v);
  bind_blob(st_insert_edge_, 3, to.id.bytes.data(), 16);
  sqlite3_bind_int64(st_insert_edge_, 4, (sqlite3_int64)to.ver.v);
  bind_text(st_insert_edge_, 5, name);
  bind_text(st_insert_edge_, 6, role);
  bind_blob(st_insert_edge_, 7, props_cbor.data(), (int)props_cbor.size());
  sqlite3_bind_int64(st_insert_edge_, 8, (sqlite3_int64)unix_ms_now());

  int rc = sqlite3_step(st_insert_edge_);
  if (rc != SQLITE_DONE) {
    sqlite3_reset(st_insert_edge_);
    sqlite3_clear_bindings(st_insert_edge_);
    return Result<void>::err(sqlite_err(db_, "insert edge failed"));
  }
  sqlite3_reset(st_insert_edge_);
  sqlite3_clear_bindings(st_insert_edge_);
  return Result<void>::ok();
}

static Result<std::vector<EdgeRecord>> read_edges(sqlite3* db, sqlite3_stmt* st) {
  std::vector<EdgeRecord> out;
  for (;;) {
    int rc = sqlite3_step(st);
    if (rc == SQLITE_ROW) {
      EdgeRecord e;
      {
        auto b = SqliteStore::col_blob(st, 0);
        if (b.size() == 16) std::memcpy(e.from.id.bytes.data(), b.data(), 16);
      }
      e.from.ver = Version{ (std::uint64_t)sqlite3_column_int64(st, 1) };
      {
        auto b = SqliteStore::col_blob(st, 2);
        if (b.size() == 16) std::memcpy(e.to.id.bytes.data(), b.data(), 16);
      }
      e.to.ver = Version{ (std::uint64_t)sqlite3_column_int64(st, 3) };
      e.name = SqliteStore::col_text(st, 4);
      e.role = SqliteStore::col_text(st, 5);
      e.props_cbor = SqliteStore::col_blob(st, 6);
      e.created_at_unix_ms = (std::uint64_t)sqlite3_column_int64(st, 7);
      out.push_back(std::move(e));
      continue;
    }
    if (rc == SQLITE_DONE) break;
    sqlite3_reset(st);
    sqlite3_clear_bindings(st);
    return Result<std::vector<EdgeRecord>>::err(sqlite_err(db, "read edges failed"));
  }
  sqlite3_reset(st);
  sqlite3_clear_bindings(st);
  return Result<std::vector<EdgeRecord>>::ok(std::move(out));
}

Result<std::vector<EdgeRecord>> SqliteStore::edges_from(ObjectRef from,
                                                        std::optional<std::string> name_filter,
                                                        std::optional<std::string> role_filter) {
  if (!db_) return Result<std::vector<EdgeRecord>>::err("db not open");
  if (!st_edges_from_ || !st_edges_from_named_ || !st_edges_from_role_ || !st_edges_from_named_role_) {
    return Result<std::vector<EdgeRecord>>::err("edges_from statement not prepared");
  }

  sqlite3_stmt* st = nullptr;
  if (name_filter && role_filter) st = st_edges_from_named_role_;
  else if (name_filter) st = st_edges_from_named_;
  else if (role_filter) st = st_edges_from_role_;
  else st = st_edges_from_;
  sqlite3_reset(st);
  sqlite3_clear_bindings(st);

  bind_blob(st, 1, from.id.bytes.data(), 16);
  sqlite3_bind_int64(st, 2, (sqlite3_int64)from.ver.v);
  if (name_filter && role_filter) {
    bind_text(st, 3, *name_filter);
    bind_text(st, 4, *role_filter);
  } else if (name_filter) {
    bind_text(st, 3, *name_filter);
  } else if (role_filter) {
    bind_text(st, 3, *role_filter);
  }

  return read_edges(db_, st);
}

Result<std::vector<EdgeRecord>> SqliteStore::edges_to(ObjectRef to,
                                                      std::optional<std::string> name_filter,
                                                      std::optional<std::string> role_filter) {
  if (!db_) return Result<std::vector<EdgeRecord>>::err("db not open");
  if (!st_edges_to_ || !st_edges_to_named_ || !st_edges_to_role_ || !st_edges_to_named_role_) {
    return Result<std::vector<EdgeRecord>>::err("edges_to statement not prepared");
  }

  sqlite3_stmt* st = nullptr;
  if (name_filter && role_filter) st = st_edges_to_named_role_;
  else if (name_filter) st = st_edges_to_named_;
  else if (role_filter) st = st_edges_to_role_;
  else st = st_edges_to_;
  sqlite3_reset(st);
  sqlite3_clear_bindings(st);

  bind_blob(st, 1, to.id.bytes.data(), 16);
  sqlite3_bind_int64(st, 2, (sqlite3_int64)to.ver.v);
  if (name_filter && role_filter) {
    bind_text(st, 3, *name_filter);
    bind_text(st, 4, *role_filter);
  } else if (name_filter) {
    bind_text(st, 3, *name_filter);
  } else if (role_filter) {
    bind_text(st, 3, *role_filter);
  }

  return read_edges(db_, st);
}

} // namespace referee
