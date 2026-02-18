#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace referee {

// -----------------------------
// Strong-ish IDs
// -----------------------------
struct ObjectID {
  std::array<std::uint8_t, 16> bytes{};

  static ObjectID random();                 // cryptographic-quality not guaranteed (v0.1)
  std::string to_hex() const;               // 32 hex chars
  static ObjectID from_hex(std::string_view hex);

  friend bool operator==(const ObjectID& a, const ObjectID& b) noexcept { return a.bytes == b.bytes; }
  friend bool operator!=(const ObjectID& a, const ObjectID& b) noexcept { return !(a == b); }
};

struct TypeID {
  std::uint64_t v{};
  friend bool operator==(TypeID a, TypeID b) noexcept { return a.v == b.v; }
  friend bool operator!=(TypeID a, TypeID b) noexcept { return a.v != b.v; }
};

struct Version {
  std::uint64_t v{};
  friend bool operator==(Version a, Version b) noexcept { return a.v == b.v; }
  friend bool operator!=(Version a, Version b) noexcept { return a.v != b.v; }
};

struct ObjectRef {
  ObjectID id{};
  Version ver{};

  friend bool operator==(const ObjectRef& a, const ObjectRef& b) noexcept {
    return a.id == b.id && a.ver == b.ver;
  }
};

// -----------------------------
// Payload
// -----------------------------
using Bytes = std::vector<std::uint8_t>;

// -----------------------------
// Records
// -----------------------------
struct ObjectRecord {
  ObjectRef ref{};
  TypeID type{};
  ObjectID definition_id{};
  Bytes payload_cbor{};
  std::uint64_t created_at_unix_ms{};
};

struct EdgeRecord {
  ObjectRef from{};
  ObjectRef to{};
  std::string name;          // optional name; empty allowed
  std::string role;          // optional role; empty allowed
  Bytes props_cbor{};        // optional props; empty allowed
  std::uint64_t created_at_unix_ms{};
};

// -----------------------------
// Errors
// -----------------------------
struct Error {
  std::string message;
};

//--------
//--------
template <typename T>
struct Result {
  std::optional<T> value;
  std::optional<Error> error;

  static Result ok(T v) { return Result{std::optional<T>(std::move(v)), std::nullopt}; }
  static Result err(std::string msg) { return Result{std::nullopt, Error{std::move(msg)}}; }

  explicit operator bool() const { return value.has_value() && !error.has_value(); }
};

// Specialization for void
template <>
struct Result<void> {
  bool ok_{true};
  std::optional<Error> error;

  static Result ok() { return Result{true, std::nullopt}; }
  static Result err(std::string msg) { return Result{false, Error{std::move(msg)}}; }

  explicit operator bool() const { return ok_ && !error.has_value(); }
};

// -----------------------------
// CBOR helpers (nlohmann::json)
// -----------------------------
Bytes cbor_from_json_string(std::string_view json_text);
std::string json_string_from_cbor(const Bytes& cbor);

// convenience for small props/payload docs
Bytes cbor_from_json_kv(std::string_view key, std::string_view value);

// -----------------------------
// Time
// -----------------------------
std::uint64_t unix_ms_now();

} // namespace referee
