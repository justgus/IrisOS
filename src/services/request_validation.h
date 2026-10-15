#pragma once

#include "services/service.h"

#include <nlohmann/json.hpp>

namespace iris::service::detail {

// Check every selector against one operation, even when IPC resolved by recipient.
inline referee::Result<referee::TypeID> operation_for(
    const ServiceDescriptor& descriptor, const MessageEnvelope& request) {
  if ((request.recipient && *request.recipient != descriptor.id) || !request.endpoint ||
      (request.endpoint->name.empty() && !request.endpoint->type)) {
    return referee::Result<referee::TypeID>::err(referee::ErrorCode::InvalidArgument,
                                                "missing or invalid service selector");
  }
  for (const auto& endpoint : descriptor.endpoints) {
    if ((!request.endpoint->name.empty() && request.endpoint->name != endpoint.name) ||
        (request.endpoint->type && request.endpoint->type != endpoint.type) ||
        !endpoint.type || request.message_type != *endpoint.type) continue;
    return referee::Result<referee::TypeID>::ok(*endpoint.type);
  }
  return referee::Result<referee::TypeID>::err(referee::ErrorCode::InvalidArgument,
                                              "inconsistent or unknown service operation");
}

inline referee::Result<nlohmann::json> object_payload(const referee::Bytes& bytes) {
  try {
    auto payload = nlohmann::json::from_cbor(bytes);
    if (payload.is_object()) return referee::Result<nlohmann::json>::ok(std::move(payload));
  } catch (const nlohmann::json::exception&) {
    return referee::Result<nlohmann::json>::err(referee::ErrorCode::InvalidArgument,
                                               "invalid CBOR payload");
  }
  return referee::Result<nlohmann::json>::err(referee::ErrorCode::InvalidArgument,
                                             "payload must be an object");
}

inline std::uint64_t positive_size(const nlohmann::json& value) {
  if (!value.is_number_unsigned() &&
      !(value.is_number_integer() && value.get<std::int64_t>() > 0)) {
    throw std::invalid_argument("size must be a positive integer");
  }
  auto size = value.get<std::uint64_t>();
  if (size == 0) throw std::invalid_argument("size must be positive");
  return size;
}

} // namespace iris::service::detail
