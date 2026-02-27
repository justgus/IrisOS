#include "referee_sqlite/sqlite_store.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace referee {
namespace {

constexpr std::uint32_t kObjTag = 0x314a424f; // "OBJ1"
constexpr std::uint32_t kEdgeTag = 0x31474445; // "EDG1"

void write_u32(std::ostream& out, std::uint32_t v) {
  std::array<std::uint8_t, 4> b{
    static_cast<std::uint8_t>(v & 0xFFu),
    static_cast<std::uint8_t>((v >> 8) & 0xFFu),
    static_cast<std::uint8_t>((v >> 16) & 0xFFu),
    static_cast<std::uint8_t>((v >> 24) & 0xFFu)
  };
  out.write(reinterpret_cast<const char*>(b.data()), b.size());
}

void write_u64(std::ostream& out, std::uint64_t v) {
  std::array<std::uint8_t, 8> b{
    static_cast<std::uint8_t>(v & 0xFFu),
    static_cast<std::uint8_t>((v >> 8) & 0xFFu),
    static_cast<std::uint8_t>((v >> 16) & 0xFFu),
    static_cast<std::uint8_t>((v >> 24) & 0xFFu),
    static_cast<std::uint8_t>((v >> 32) & 0xFFu),
    static_cast<std::uint8_t>((v >> 40) & 0xFFu),
    static_cast<std::uint8_t>((v >> 48) & 0xFFu),
    static_cast<std::uint8_t>((v >> 56) & 0xFFu)
  };
  out.write(reinterpret_cast<const char*>(b.data()), b.size());
}

bool read_exact(std::istream& in, void* dst, std::size_t n) {
  in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
  return in.good();
}

bool read_u32(std::istream& in, std::uint32_t* out) {
  std::array<std::uint8_t, 4> b{};
  if (!read_exact(in, b.data(), b.size())) return false;
  *out = (std::uint32_t)b[0]
       | (std::uint32_t(b[1]) << 8)
       | (std::uint32_t(b[2]) << 16)
       | (std::uint32_t(b[3]) << 24);
  return true;
}

bool read_u64(std::istream& in, std::uint64_t* out) {
  std::array<std::uint8_t, 8> b{};
  if (!read_exact(in, b.data(), b.size())) return false;
  *out = (std::uint64_t)b[0]
       | (std::uint64_t(b[1]) << 8)
       | (std::uint64_t(b[2]) << 16)
       | (std::uint64_t(b[3]) << 24)
       | (std::uint64_t(b[4]) << 32)
       | (std::uint64_t(b[5]) << 40)
       | (std::uint64_t(b[6]) << 48)
       | (std::uint64_t(b[7]) << 56);
  return true;
}

std::string key_object_id(const ObjectID& id, std::uint64_t ver) {
  return id.to_hex() + ":" + std::to_string(ver);
}

std::string key_type_id(const TypeID& type, const ObjectID& id, std::uint64_t ver) {
  return std::to_string(type.v) + ":" + id.to_hex() + ":" + std::to_string(ver);
}

std::string key_edge_from(const EdgeRecord& rec) {
  return rec.from.id.to_hex() + ":" + std::to_string(rec.from.ver.v) + ":" + rec.name + ":" + rec.role
         + ":" + rec.to.id.to_hex() + ":" + std::to_string(rec.to.ver.v);
}

std::string key_edge_to(const EdgeRecord& rec) {
  return rec.to.id.to_hex() + ":" + std::to_string(rec.to.ver.v) + ":" + rec.name + ":" + rec.role
         + ":" + rec.from.id.to_hex() + ":" + std::to_string(rec.from.ver.v);
}

void append_index(std::ofstream& out, const std::string& key, std::uint64_t offset) {
  if (!out.is_open()) return;
  out << key << '\t' << offset << '\n';
}

} // namespace

std::size_t SqliteStore::ObjectIDHash::operator()(const ObjectID& id) const noexcept {
  std::size_t h = 0xcbf29ce484222325ULL;
  for (auto b : id.bytes) {
    h ^= static_cast<std::size_t>(b);
    h *= 0x100000001b3ULL;
  }
  return h;
}

std::size_t SqliteStore::ObjectRefKeyHash::operator()(const ObjectRefKey& key) const noexcept {
  std::size_t h = ObjectIDHash{}(key.id);
  h ^= std::hash<std::uint64_t>{}(key.ver.v) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
  return h;
}

SqliteStore::SqliteStore(SqliteConfig cfg) : cfg_(std::move(cfg)) {}
SqliteStore::~SqliteStore() { (void)close(); }

std::string SqliteStore::store_dir_from_filename(std::string_view filename) {
  if (filename == ":memory:") return {};
  std::string base(filename);
  return base + ".segments";
}

std::string SqliteStore::base_dir() const {
  return store_dir_from_filename(cfg_.filename);
}

Result<void> SqliteStore::open() {
  if (open_) return Result<void>::ok();
  memory_only_ = (cfg_.filename == ":memory:");
  if (memory_only_) {
    open_ = true;
    return Result<void>::ok();
  }

  const auto base = base_dir();
  const auto segments_dir = std::filesystem::path(base) / "segments";
  const auto indexes_dir = std::filesystem::path(base) / "indexes";

  std::error_code ec;
  std::filesystem::create_directories(segments_dir, ec);
  if (ec) return Result<void>::err("failed to create segments directory");
  std::filesystem::create_directories(indexes_dir, ec);
  if (ec) return Result<void>::err("failed to create indexes directory");

  const auto obj_path = segments_dir / "objects.seg";
  const auto edge_path = segments_dir / "edges.seg";

  object_seg_.open(obj_path, std::ios::binary | std::ios::app);
  if (!object_seg_) return Result<void>::err("failed to open objects.seg");
  edge_seg_.open(edge_path, std::ios::binary | std::ios::app);
  if (!edge_seg_) return Result<void>::err("failed to open edges.seg");

  idx_objects_by_id_.open(indexes_dir / "objects_by_id.idx", std::ios::app);
  idx_objects_by_type_.open(indexes_dir / "objects_by_type.idx", std::ios::app);
  idx_edges_from_.open(indexes_dir / "edges_from.idx", std::ios::app);
  idx_edges_to_.open(indexes_dir / "edges_to.idx", std::ios::app);

  auto r = load_segments();
  if (!r) return r;

  open_ = true;
  return Result<void>::ok();
}

Result<void> SqliteStore::close() {
  if (!open_) return Result<void>::ok();
  if (object_seg_.is_open()) object_seg_.close();
  if (edge_seg_.is_open()) edge_seg_.close();
  if (idx_objects_by_id_.is_open()) idx_objects_by_id_.close();
  if (idx_objects_by_type_.is_open()) idx_objects_by_type_.close();
  if (idx_edges_from_.is_open()) idx_edges_from_.close();
  if (idx_edges_to_.is_open()) idx_edges_to_.close();
  open_ = false;
  return Result<void>::ok();
}

Result<void> SqliteStore::ensure_schema() {
  return open();
}

Result<void> SqliteStore::begin() {
  if (in_txn_) return Result<void>::err("transaction already open");
  in_txn_ = true;
  pending_objects_.clear();
  pending_edges_.clear();
  return Result<void>::ok();
}

Result<void> SqliteStore::commit() {
  if (!in_txn_) return Result<void>::ok();
  for (const auto& rec : pending_objects_) {
    auto r = append_object(rec);
    if (!r) return r;
    index_object(rec);
  }
  for (const auto& rec : pending_edges_) {
    auto r = append_edge(rec);
    if (!r) return r;
    index_edge(rec);
  }
  pending_objects_.clear();
  pending_edges_.clear();
  in_txn_ = false;
  return Result<void>::ok();
}

Result<void> SqliteStore::rollback() {
  if (!in_txn_) return Result<void>::ok();
  pending_objects_.clear();
  pending_edges_.clear();
  in_txn_ = false;
  return Result<void>::ok();
}

Result<ObjectRecord> SqliteStore::create_object(TypeID type, ObjectID definition_id,
                                                const Bytes& payload_cbor) {
  return create_object_with_id(ObjectID::random(), type, definition_id, payload_cbor);
}

Result<ObjectRecord> SqliteStore::create_object_with_id(ObjectID object_id, TypeID type,
                                                        ObjectID definition_id,
                                                        const Bytes& payload_cbor) {
  if (!open_) return Result<ObjectRecord>::err("store not open");

  ObjectRecord rec;
  rec.ref.id = object_id;
  rec.ref.ver = Version{1};
  rec.type = type;
  rec.definition_id = definition_id;
  rec.payload_cbor = payload_cbor;
  rec.created_at_unix_ms = unix_ms_now();

  if (in_txn_) {
    pending_objects_.push_back(rec);
  } else {
    auto r = append_object(rec);
    if (!r) return Result<ObjectRecord>::err(r.error->message);
    index_object(rec);
  }

  return Result<ObjectRecord>::ok(std::move(rec));
}

Result<std::optional<ObjectRecord>> SqliteStore::get_object(ObjectRef ref) {
  if (!open_) return Result<std::optional<ObjectRecord>>::err("store not open");
  ObjectRefKey key{ref.id, ref.ver};
  auto it = objects_by_ref_.find(key);
  if (it == objects_by_ref_.end()) return Result<std::optional<ObjectRecord>>::ok(std::nullopt);
  return Result<std::optional<ObjectRecord>>::ok(std::optional<ObjectRecord>(it->second));
}

Result<std::optional<ObjectRecord>> SqliteStore::get_latest(ObjectID id) {
  if (!open_) return Result<std::optional<ObjectRecord>>::err("store not open");
  auto it = latest_by_id_.find(id);
  if (it == latest_by_id_.end()) return Result<std::optional<ObjectRecord>>::ok(std::nullopt);
  return Result<std::optional<ObjectRecord>>::ok(std::optional<ObjectRecord>(it->second));
}

Result<std::vector<ObjectRecord>> SqliteStore::list_by_type(TypeID type) {
  if (!open_) return Result<std::vector<ObjectRecord>>::err("store not open");
  auto it = objects_by_type_.find(type);
  if (it == objects_by_type_.end()) return Result<std::vector<ObjectRecord>>::ok({});
  return Result<std::vector<ObjectRecord>>::ok(it->second);
}

Result<void> SqliteStore::add_edge(ObjectRef from, ObjectRef to, std::string name, std::string role,
                                   const Bytes& props_cbor) {
  if (!open_) return Result<void>::err("store not open");

  EdgeRecord rec;
  rec.from = from;
  rec.to = to;
  rec.name = std::move(name);
  rec.role = std::move(role);
  rec.props_cbor = props_cbor;
  rec.created_at_unix_ms = unix_ms_now();

  if (in_txn_) {
    pending_edges_.push_back(rec);
  } else {
    auto r = append_edge(rec);
    if (!r) return r;
    index_edge(rec);
  }

  return Result<void>::ok();
}

Result<std::vector<EdgeRecord>> SqliteStore::edges_from(ObjectRef from,
                                                        std::optional<std::string> name_filter,
                                                        std::optional<std::string> role_filter) {
  if (!open_) return Result<std::vector<EdgeRecord>>::err("store not open");
  ObjectRefKey key{from.id, from.ver};
  auto it = edges_from_.find(key);
  if (it == edges_from_.end()) return Result<std::vector<EdgeRecord>>::ok({});

  std::vector<EdgeRecord> out;
  for (const auto& e : it->second) {
    if (name_filter && e.name != *name_filter) continue;
    if (role_filter && e.role != *role_filter) continue;
    out.push_back(e);
  }
  return Result<std::vector<EdgeRecord>>::ok(std::move(out));
}

Result<std::vector<EdgeRecord>> SqliteStore::edges_to(ObjectRef to,
                                                      std::optional<std::string> name_filter,
                                                      std::optional<std::string> role_filter) {
  if (!open_) return Result<std::vector<EdgeRecord>>::err("store not open");
  ObjectRefKey key{to.id, to.ver};
  auto it = edges_to_.find(key);
  if (it == edges_to_.end()) return Result<std::vector<EdgeRecord>>::ok({});

  std::vector<EdgeRecord> out;
  for (const auto& e : it->second) {
    if (name_filter && e.name != *name_filter) continue;
    if (role_filter && e.role != *role_filter) continue;
    out.push_back(e);
  }
  return Result<std::vector<EdgeRecord>>::ok(std::move(out));
}

Result<void> SqliteStore::append_object(const ObjectRecord& rec) {
  if (memory_only_) return Result<void>::ok();
  if (!object_seg_.is_open()) return Result<void>::err("objects segment not open");

  const auto offset = static_cast<std::uint64_t>(object_seg_.tellp());

  write_u32(object_seg_, kObjTag);
  write_u32(object_seg_, static_cast<std::uint32_t>(rec.payload_cbor.size()));
  write_u64(object_seg_, rec.ref.ver.v);
  write_u64(object_seg_, rec.type.v);
  write_u64(object_seg_, rec.created_at_unix_ms);
  object_seg_.write(reinterpret_cast<const char*>(rec.ref.id.bytes.data()), rec.ref.id.bytes.size());
  object_seg_.write(reinterpret_cast<const char*>(rec.definition_id.bytes.data()),
                    rec.definition_id.bytes.size());
  if (!rec.payload_cbor.empty()) {
    object_seg_.write(reinterpret_cast<const char*>(rec.payload_cbor.data()),
                      static_cast<std::streamsize>(rec.payload_cbor.size()));
  }
  object_seg_.flush();

  append_index(idx_objects_by_id_, key_object_id(rec.ref.id, rec.ref.ver.v), offset);
  append_index(idx_objects_by_type_, key_type_id(rec.type, rec.ref.id, rec.ref.ver.v), offset);

  return Result<void>::ok();
}

Result<void> SqliteStore::append_edge(const EdgeRecord& rec) {
  if (memory_only_) return Result<void>::ok();
  if (!edge_seg_.is_open()) return Result<void>::err("edges segment not open");

  const auto offset = static_cast<std::uint64_t>(edge_seg_.tellp());

  write_u32(edge_seg_, kEdgeTag);
  write_u32(edge_seg_, static_cast<std::uint32_t>(rec.name.size()));
  write_u32(edge_seg_, static_cast<std::uint32_t>(rec.role.size()));
  write_u32(edge_seg_, static_cast<std::uint32_t>(rec.props_cbor.size()));
  write_u64(edge_seg_, rec.created_at_unix_ms);
  edge_seg_.write(reinterpret_cast<const char*>(rec.from.id.bytes.data()), rec.from.id.bytes.size());
  write_u64(edge_seg_, rec.from.ver.v);
  edge_seg_.write(reinterpret_cast<const char*>(rec.to.id.bytes.data()), rec.to.id.bytes.size());
  write_u64(edge_seg_, rec.to.ver.v);
  if (!rec.name.empty()) edge_seg_.write(rec.name.data(), static_cast<std::streamsize>(rec.name.size()));
  if (!rec.role.empty()) edge_seg_.write(rec.role.data(), static_cast<std::streamsize>(rec.role.size()));
  if (!rec.props_cbor.empty()) {
    edge_seg_.write(reinterpret_cast<const char*>(rec.props_cbor.data()),
                    static_cast<std::streamsize>(rec.props_cbor.size()));
  }
  edge_seg_.flush();

  append_index(idx_edges_from_, key_edge_from(rec), offset);
  append_index(idx_edges_to_, key_edge_to(rec), offset);

  return Result<void>::ok();
}

void SqliteStore::index_object(const ObjectRecord& rec) {
  ObjectRefKey key{rec.ref.id, rec.ref.ver};
  objects_by_ref_[key] = rec;
  latest_by_id_[rec.ref.id] = rec;
  objects_by_type_[rec.type].push_back(rec);
}

void SqliteStore::index_edge(const EdgeRecord& rec) {
  ObjectRefKey from_key{rec.from.id, rec.from.ver};
  ObjectRefKey to_key{rec.to.id, rec.to.ver};
  edges_from_[from_key].push_back(rec);
  edges_to_[to_key].push_back(rec);
}

Result<void> SqliteStore::load_segments() {
  if (memory_only_) return Result<void>::ok();

  const auto base = base_dir();
  const auto segments_dir = std::filesystem::path(base) / "segments";
  const auto obj_path = segments_dir / "objects.seg";
  const auto edge_path = segments_dir / "edges.seg";

  if (std::filesystem::exists(obj_path)) {
    std::ifstream in(obj_path, std::ios::binary);
    while (in.good()) {
      std::uint32_t tag = 0;
      if (!read_u32(in, &tag)) break;
      if (tag != kObjTag) return Result<void>::err("invalid object segment tag");

      std::uint32_t payload_size = 0;
      std::uint64_t ver = 0;
      std::uint64_t type = 0;
      std::uint64_t created = 0;
      if (!read_u32(in, &payload_size)) return Result<void>::err("object segment read failed");
      if (!read_u64(in, &ver)) return Result<void>::err("object segment read failed");
      if (!read_u64(in, &type)) return Result<void>::err("object segment read failed");
      if (!read_u64(in, &created)) return Result<void>::err("object segment read failed");

      ObjectRecord rec;
      if (!read_exact(in, rec.ref.id.bytes.data(), rec.ref.id.bytes.size())) {
        return Result<void>::err("object segment read failed");
      }
      if (!read_exact(in, rec.definition_id.bytes.data(), rec.definition_id.bytes.size())) {
        return Result<void>::err("object segment read failed");
      }

      rec.ref.ver = Version{ver};
      rec.type = TypeID{type};
      rec.created_at_unix_ms = created;
      rec.payload_cbor.resize(payload_size);
      if (payload_size > 0 && !read_exact(in, rec.payload_cbor.data(), payload_size)) {
        return Result<void>::err("object payload read failed");
      }

      index_object(rec);
    }
  }

  if (std::filesystem::exists(edge_path)) {
    std::ifstream in(edge_path, std::ios::binary);
    while (in.good()) {
      std::uint32_t tag = 0;
      if (!read_u32(in, &tag)) break;
      if (tag != kEdgeTag) return Result<void>::err("invalid edge segment tag");

      std::uint32_t name_len = 0;
      std::uint32_t role_len = 0;
      std::uint32_t props_len = 0;
      std::uint64_t created = 0;
      if (!read_u32(in, &name_len)) return Result<void>::err("edge segment read failed");
      if (!read_u32(in, &role_len)) return Result<void>::err("edge segment read failed");
      if (!read_u32(in, &props_len)) return Result<void>::err("edge segment read failed");
      if (!read_u64(in, &created)) return Result<void>::err("edge segment read failed");

      EdgeRecord rec;
      if (!read_exact(in, rec.from.id.bytes.data(), rec.from.id.bytes.size())) {
        return Result<void>::err("edge segment read failed");
      }
      std::uint64_t from_ver = 0;
      if (!read_u64(in, &from_ver)) return Result<void>::err("edge segment read failed");
      rec.from.ver = Version{from_ver};

      if (!read_exact(in, rec.to.id.bytes.data(), rec.to.id.bytes.size())) {
        return Result<void>::err("edge segment read failed");
      }
      std::uint64_t to_ver = 0;
      if (!read_u64(in, &to_ver)) return Result<void>::err("edge segment read failed");
      rec.to.ver = Version{to_ver};

      rec.created_at_unix_ms = created;
      rec.name.resize(name_len);
      rec.role.resize(role_len);
      rec.props_cbor.resize(props_len);

      if (name_len > 0 && !read_exact(in, rec.name.data(), name_len)) {
        return Result<void>::err("edge name read failed");
      }
      if (role_len > 0 && !read_exact(in, rec.role.data(), role_len)) {
        return Result<void>::err("edge role read failed");
      }
      if (props_len > 0 && !read_exact(in, rec.props_cbor.data(), props_len)) {
        return Result<void>::err("edge props read failed");
      }

      index_edge(rec);
    }
  }

  return Result<void>::ok();
}

} // namespace referee
