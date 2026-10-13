#include "services/host_records.h"

#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace iris::service {

std::string_view service_host_state_name(ServiceHostState state) {
  switch (state) {
  case ServiceHostState::Created: return "created";
  case ServiceHostState::Running: return "running";
  case ServiceHostState::Stopped: return "stopped";
  }
  return "unknown";
}

namespace {

using Json = nlohmann::json;

bool valid_grants(const std::vector<std::string>& grants) {
  std::unordered_set<std::string> seen;
  for (const auto& grant : grants) {
    if (grant.empty() || !seen.insert(grant).second) return false;
  }
  return true;
}

Json encode_descriptor(const ServiceDescriptor& descriptor) {
  Json endpoints = Json::array();
  for (const auto& endpoint : descriptor.endpoints) {
    Json encoded{{"name", endpoint.name}, {"required_grants", endpoint.required_grants}};
    if (endpoint.type) encoded["type"] = endpoint.type->v;
    endpoints.push_back(std::move(encoded));
  }
  return Json{{"id", descriptor.id.to_hex()}, {"type", descriptor.type.v},
              {"name", descriptor.name}, {"required_grants", descriptor.required_grants},
              {"endpoints", std::move(endpoints)}};
}

std::uint64_t unsigned_field(const Json& object, const char* name) {
  const auto& value = object.at(name);
  if (!value.is_number_unsigned()) throw std::runtime_error(std::string("invalid ") + name);
  return value.get<std::uint64_t>();
}

referee::Result<ServiceHostRecord> decode_record(const referee::ObjectRecord& object) {
  try {
    const auto payload = Json::from_cbor(object.payload_cbor);
    if (unsigned_field(payload, "schema_version") != 1) {
      throw std::runtime_error("unsupported service host schema version");
    }
    const auto& encoded = payload.at("descriptor");
    ServiceHostRecord record;
    record.ref = object.ref;
    record.descriptor.id = referee::ObjectID::from_hex(encoded.at("id").get<std::string>());
    record.descriptor.type = referee::TypeID{unsigned_field(encoded, "type")};
    record.descriptor.name = encoded.at("name").get<std::string>();
    record.descriptor.required_grants = encoded.at("required_grants").get<std::vector<std::string>>();
    if (!encoded.at("endpoints").is_array()) throw std::runtime_error("invalid endpoints");
    for (const auto& item : encoded.at("endpoints")) {
      Endpoint endpoint;
      endpoint.name = item.at("name").get<std::string>();
      if (item.contains("type")) endpoint.type = referee::TypeID{unsigned_field(item, "type")};
      endpoint.required_grants = item.at("required_grants").get<std::vector<std::string>>();
      record.descriptor.endpoints.push_back(std::move(endpoint));
    }
    auto valid = detail::validate_service_descriptor(record.descriptor);
    if (!valid) throw std::runtime_error(valid.error->message);
    const auto state = payload.at("state").get<std::string>();
    if (state == "created") record.state = ServiceHostState::Created;
    else if (state == "running") record.state = ServiceHostState::Running;
    else if (state == "stopped") record.state = ServiceHostState::Stopped;
    else throw std::runtime_error("invalid service host state");
    record.sequence = unsigned_field(payload, "sequence");
    record.generation = unsigned_field(payload, "generation");
    if (payload.contains("previous_record")) {
      record.previous_record = referee::ObjectID::from_hex(payload.at("previous_record").get<std::string>());
    }
    return referee::Result<ServiceHostRecord>::ok(std::move(record));
  } catch (const std::exception& ex) {
    return referee::Result<ServiceHostRecord>::err(referee::ErrorCode::CorruptData, ex.what());
  }
}

} // namespace

namespace detail {

referee::Result<void> validate_service_descriptor(const ServiceDescriptor& descriptor) {
  if (descriptor.id == referee::ObjectID{} || descriptor.type.v == 0 || descriptor.name.empty()
      || !valid_grants(descriptor.required_grants)) {
    return referee::Result<void>::err(referee::ErrorCode::InvalidArgument, "invalid service descriptor");
  }
  std::unordered_set<std::string> names;
  std::unordered_set<std::uint64_t> types;
  for (const auto& endpoint : descriptor.endpoints) {
    if ((endpoint.name.empty() && !endpoint.type) || (endpoint.type && endpoint.type->v == 0)
        || (!endpoint.name.empty() && !names.insert(endpoint.name).second)
        || (endpoint.type && !types.insert(endpoint.type->v).second)
        || !valid_grants(endpoint.required_grants)) {
      return referee::Result<void>::err(referee::ErrorCode::InvalidArgument, "invalid service endpoint");
    }
  }
  return referee::Result<void>::ok();
}

referee::Result<std::vector<ServiceHostRecord>> load_service_hosts(referee::SqliteStore& store) {
  auto objects = store.list_by_type(kServiceHostRecordType);
  if (!objects) return referee::Result<std::vector<ServiceHostRecord>>::err(objects.error.value());
  std::map<std::string, std::vector<ServiceHostRecord>> histories;
  for (const auto& object : objects.value.value()) {
    auto decoded = decode_record(object);
    if (!decoded) return referee::Result<std::vector<ServiceHostRecord>>::err(decoded.error.value());
    histories[decoded.value->descriptor.id.to_hex()].push_back(std::move(decoded.value.value()));
  }
  std::vector<ServiceHostRecord> latest;
  for (auto& [id, history] : histories) {
    (void)id;
    std::sort(history.begin(), history.end(), [](const auto& lhs, const auto& rhs) {
      return lhs.sequence < rhs.sequence;
    });
    for (std::size_t i = 0; i < history.size(); ++i) {
      const auto& current = history[i];
      bool valid = current.sequence == i + 1 && current.descriptor == history.front().descriptor;
      if (i == 0) {
        valid = valid && !current.previous_record && current.generation == 0
                && current.state == ServiceHostState::Created
                && current.ref.id == current.descriptor.id;
      } else {
        const auto& prior = history[i - 1];
        valid = valid && current.previous_record == prior.ref.id
                && current.state != ServiceHostState::Created;
        if (current.state == ServiceHostState::Running) {
          valid = valid && prior.generation != std::numeric_limits<std::uint64_t>::max()
                  && current.generation == prior.generation + 1;
        } else {
          valid = valid && current.generation == prior.generation;
        }
      }
      if (!valid) {
        return referee::Result<std::vector<ServiceHostRecord>>::err(
            referee::ErrorCode::CorruptData, "invalid service host history");
      }
    }
    latest.push_back(std::move(history.back()));
  }
  return referee::Result<std::vector<ServiceHostRecord>>::ok(std::move(latest));
}

referee::Result<ServiceHostRecord> append_service_host(
    referee::SqliteStore& store, const ServiceDescriptor& descriptor,
    ServiceHostState state, std::uint64_t generation,
    const std::optional<ServiceHostRecord>& previous) {
  if (previous && previous->sequence == std::numeric_limits<std::uint64_t>::max()) {
    return referee::Result<ServiceHostRecord>::err(referee::ErrorCode::FailedPrecondition,
                                                  "service host history exhausted");
  }
  ServiceHostRecord record;
  record.descriptor = descriptor;
  record.state = state;
  record.sequence = previous ? previous->sequence + 1 : 1;
  record.generation = generation;
  Json payload{{"schema_version", std::uint64_t{1}}, {"descriptor", encode_descriptor(descriptor)},
               {"state", service_host_state_name(state)}, {"sequence", record.sequence},
               {"generation", generation}};
  if (previous) {
    record.previous_record = previous->ref.id;
    payload["previous_record"] = previous->ref.id.to_hex();
  }
  auto object = previous
      ? store.create_object(kServiceHostRecordType, {}, Json::to_cbor(payload))
      : store.create_object_with_id(descriptor.id, kServiceHostRecordType, {}, Json::to_cbor(payload));
  if (!object) return referee::Result<ServiceHostRecord>::err(object.error.value());
  record.ref = object.value->ref;
  return referee::Result<ServiceHostRecord>::ok(std::move(record));
}

} // namespace detail
} // namespace iris::service
