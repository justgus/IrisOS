#include "referee/referee.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace referee {

static std::uint8_t hexval(char c) {
  if (c >= '0' && c <= '9') return (std::uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f') return (std::uint8_t)(10 + (c - 'a'));
  if (c >= 'A' && c <= 'F') return (std::uint8_t)(10 + (c - 'A'));
  throw std::runtime_error("invalid hex digit");
}

ObjectID ObjectID::random() {
  // v0.1: uses random_device; on Debian this is usually backed by urandom
  // Later: use getrandom() directly or libsodium.
  std::random_device rd;
  ObjectID id;
  for (auto& b : id.bytes) b = (std::uint8_t)(rd() & 0xFFu);
  // Set UUID v4-ish bits (optional, but helps with debugging expectations)
  id.bytes[6] = (std::uint8_t)((id.bytes[6] & 0x0F) | 0x40);
  id.bytes[8] = (std::uint8_t)((id.bytes[8] & 0x3F) | 0x80);
  return id;
}

std::string ObjectID::to_hex() const {
  std::ostringstream os;
  os << std::hex << std::setfill('0');
  for (auto b : bytes) os << std::setw(2) << (int)b;
  return os.str();
}

ObjectID ObjectID::from_hex(std::string_view hex) {
  if (hex.size() != 32) throw std::runtime_error("ObjectID hex must be 32 chars");
  ObjectID id;
  for (size_t i = 0; i < 16; ++i) {
    auto hi = hexval(hex[2 * i]);
    auto lo = hexval(hex[2 * i + 1]);
    id.bytes[i] = (std::uint8_t)((hi << 4) | lo);
  }
  return id;
}

std::uint64_t unix_ms_now() {
  using namespace std::chrono;
  auto now = time_point_cast<milliseconds>(system_clock::now());
  return (std::uint64_t)now.time_since_epoch().count();
}

Bytes cbor_from_json_string(std::string_view json_text) {
  // Throws on parse error.
  nlohmann::json j = nlohmann::json::parse(json_text.begin(), json_text.end());
  return nlohmann::json::to_cbor(j);
}

std::string json_string_from_cbor(const Bytes& cbor) {
  nlohmann::json j = nlohmann::json::from_cbor(cbor);
  return j.dump();
}

Bytes cbor_from_json_kv(std::string_view key, std::string_view value) {
  nlohmann::json j;
  j[std::string(key)] = std::string(value);
  return nlohmann::json::to_cbor(j);
}

} // namespace referee