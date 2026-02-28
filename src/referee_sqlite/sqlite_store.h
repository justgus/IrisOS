#pragma once

#include "referee/referee.h"

#ifdef fail
#undef fail
#endif

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace referee {

struct SqliteConfig {
  std::string filename;     // base path for segment store (":memory:" for in-memory)
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

  // Storage management
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

private:
  struct ObjectRefKey {
    ObjectID id{};
    Version ver{};

    friend bool operator==(const ObjectRefKey& a, const ObjectRefKey& b) noexcept {
      return a.id == b.id && a.ver == b.ver;
    }
  };

  struct ObjectIDHash {
    std::size_t operator()(const ObjectID& id) const noexcept;
  };

  struct ObjectRefKeyHash {
    std::size_t operator()(const ObjectRefKey& key) const noexcept;
  };

  struct TypeIDHash {
    std::size_t operator()(const TypeID& t) const noexcept {
      return std::hash<std::uint64_t>{}(t.v);
    }
  };

  Result<void> load_segments();
  Result<void> rebuild_indexes();
  Result<void> append_object(const ObjectRecord& rec);
  Result<void> append_edge(const EdgeRecord& rec);
  void index_object(const ObjectRecord& rec);
  void index_edge(const EdgeRecord& rec);

  std::string base_dir() const;
  static std::string store_dir_from_filename(std::string_view filename);

private:
  SqliteConfig cfg_;
  bool open_{false};
  bool memory_only_{false};
  bool in_txn_{false};

  std::ofstream object_seg_;
  std::ofstream edge_seg_;
  std::ofstream idx_objects_by_id_;
  std::ofstream idx_objects_by_type_;
  std::ofstream idx_edges_from_;
  std::ofstream idx_edges_to_;

  std::vector<ObjectRecord> pending_objects_;
  std::vector<EdgeRecord> pending_edges_;

  std::unordered_map<ObjectRefKey, ObjectRecord, ObjectRefKeyHash> objects_by_ref_;
  std::unordered_map<ObjectID, ObjectRecord, ObjectIDHash> latest_by_id_;
  std::unordered_map<TypeID, std::vector<ObjectRecord>, TypeIDHash> objects_by_type_;
  std::unordered_map<ObjectRefKey, std::vector<EdgeRecord>, ObjectRefKeyHash> edges_from_;
  std::unordered_map<ObjectRefKey, std::vector<EdgeRecord>, ObjectRefKeyHash> edges_to_;
};

} // namespace referee
