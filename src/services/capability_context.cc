#include "services/capability_context.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <exception>
#include <unordered_set>

namespace iris::service {

namespace {

referee::Result<CapabilityContext> validate_context(CapabilityContext context) {
  std::unordered_set<std::string> seen;
  for (const auto& grant : context.grants) {
    if (grant.name.empty()) {
      return referee::Result<CapabilityContext>::err(
          referee::ErrorCode::InvalidArgument,
          "capability grant name is empty");
    }
    if (!seen.insert(grant.name).second) {
      return referee::Result<CapabilityContext>::err(
          referee::ErrorCode::InvalidArgument,
          "duplicate capability grant: " + grant.name);
    }
  }

  std::sort(context.grants.begin(), context.grants.end(),
            [](const CapabilityGrant& lhs, const CapabilityGrant& rhs) {
              return lhs.name < rhs.name;
            });
  return referee::Result<CapabilityContext>::ok(std::move(context));
}

referee::Result<SandboxIdentity> validate_sandbox(SandboxIdentity sandbox) {
  if (sandbox.name.empty()) {
    return referee::Result<SandboxIdentity>::err(referee::ErrorCode::InvalidArgument,
                                                 "sandbox name is empty");
  }

  std::sort(sandbox.subjects.begin(), sandbox.subjects.end(),
            [](const referee::ObjectID& lhs, const referee::ObjectID& rhs) {
              return lhs.to_hex() < rhs.to_hex();
            });
  auto duplicate = std::adjacent_find(sandbox.subjects.begin(), sandbox.subjects.end());
  if (duplicate != sandbox.subjects.end()) {
    return referee::Result<SandboxIdentity>::err(
        referee::ErrorCode::InvalidArgument,
        "duplicate sandbox subject: " + duplicate->to_hex());
  }

  return referee::Result<SandboxIdentity>::ok(std::move(sandbox));
}

nlohmann::json context_to_json(const CapabilityContext& context) {
  nlohmann::json j;
  j["subject"] = context.subject.to_hex();
  if (context.sandbox.has_value()) j["sandbox"] = context.sandbox->to_hex();
  j["grants"] = nlohmann::json::array();
  for (const auto& grant : context.grants) {
    j["grants"].push_back(grant.name);
  }
  return j;
}

referee::Result<SandboxIdentity> sandbox_from_json(referee::ObjectID id,
                                                   const nlohmann::json& j) {
  if (!j.is_object()) {
    return referee::Result<SandboxIdentity>::err(referee::ErrorCode::CorruptData,
                                                 "sandbox payload must be an object");
  }
  if (!j.contains("name")) {
    return referee::Result<SandboxIdentity>::err(referee::ErrorCode::CorruptData,
                                                 "sandbox missing name");
  }
  if (!j.contains("subjects") || !j.at("subjects").is_array()) {
    return referee::Result<SandboxIdentity>::err(referee::ErrorCode::CorruptData,
                                                 "sandbox subjects must be an array");
  }

  SandboxIdentity sandbox;
  sandbox.id = id;
  try {
    sandbox.name = j.at("name").get<std::string>();
    for (const auto& item : j.at("subjects")) {
      sandbox.subjects.push_back(referee::ObjectID::from_hex(item.get<std::string>()));
    }
  } catch (const std::exception& ex) {
    return referee::Result<SandboxIdentity>::err(referee::ErrorCode::CorruptData, ex.what());
  }

  return validate_sandbox(std::move(sandbox));
}

nlohmann::json sandbox_to_json(const SandboxIdentity& sandbox) {
  nlohmann::json j;
  j["name"] = sandbox.name;
  j["subjects"] = nlohmann::json::array();
  for (const auto& subject : sandbox.subjects) {
    j["subjects"].push_back(subject.to_hex());
  }
  return j;
}

referee::Result<CapabilityContext> context_from_json(referee::ObjectID id,
                                                     const nlohmann::json& j) {
  if (!j.is_object()) {
    return referee::Result<CapabilityContext>::err(
        referee::ErrorCode::CorruptData,
        "capability context payload must be an object");
  }
  if (!j.contains("subject")) {
    return referee::Result<CapabilityContext>::err(
        referee::ErrorCode::CorruptData,
        "capability context missing subject");
  }
  if (!j.contains("grants") || !j.at("grants").is_array()) {
    return referee::Result<CapabilityContext>::err(
        referee::ErrorCode::CorruptData,
        "capability context grants must be an array");
  }

  CapabilityContext context;
  context.id = id;
  try {
    context.subject = referee::ObjectID::from_hex(j.at("subject").get<std::string>());
    if (j.contains("sandbox")) {
      context.sandbox = referee::ObjectID::from_hex(j.at("sandbox").get<std::string>());
    }
    for (const auto& item : j.at("grants")) {
      context.grants.push_back(CapabilityGrant{item.get<std::string>()});
    }
  } catch (const std::exception& ex) {
    return referee::Result<CapabilityContext>::err(referee::ErrorCode::CorruptData, ex.what());
  }

  return validate_context(std::move(context));
}

referee::Result<CapabilityContextRecord> record_from_object(const referee::ObjectRecord& rec) {
  try {
    auto json = nlohmann::json::from_cbor(rec.payload_cbor);
    auto contextR = context_from_json(rec.ref.id, json);
    if (!contextR) {
      return referee::Result<CapabilityContextRecord>::err(contextR.error.value());
    }
    CapabilityContextRecord out;
    out.ref = rec.ref;
    out.context = std::move(contextR.value.value());
    return referee::Result<CapabilityContextRecord>::ok(std::move(out));
  } catch (const std::exception& ex) {
    return referee::Result<CapabilityContextRecord>::err(referee::ErrorCode::CorruptData,
                                                         ex.what());
  }
}

referee::Result<SandboxIdentityRecord> sandbox_record_from_object(
    const referee::ObjectRecord& rec) {
  try {
    auto json = nlohmann::json::from_cbor(rec.payload_cbor);
    auto sandboxR = sandbox_from_json(rec.ref.id, json);
    if (!sandboxR) {
      return referee::Result<SandboxIdentityRecord>::err(sandboxR.error.value());
    }
    SandboxIdentityRecord out;
    out.ref = rec.ref;
    out.sandbox = std::move(sandboxR.value.value());
    return referee::Result<SandboxIdentityRecord>::ok(std::move(out));
  } catch (const std::exception& ex) {
    return referee::Result<SandboxIdentityRecord>::err(referee::ErrorCode::CorruptData,
                                                       ex.what());
  }
}

} // namespace

CapabilityContextStore::CapabilityContextStore(referee::SqliteStore& store) : store_(store) {}

referee::Result<SandboxIdentityRecord> CapabilityContextStore::persist_sandbox(
    const SandboxIdentity& sandbox) {
  auto normalizedR = validate_sandbox(sandbox);
  if (!normalizedR) {
    return referee::Result<SandboxIdentityRecord>::err(normalizedR.error.value());
  }

  const auto& normalized = normalizedR.value.value();
  auto payload = nlohmann::json::to_cbor(sandbox_to_json(normalized));
  auto recR = store_.create_object_with_id(normalized.id,
                                           kSandboxIdentityType,
                                           referee::ObjectID{},
                                           payload);
  if (!recR) return referee::Result<SandboxIdentityRecord>::err(recR.error.value());

  SandboxIdentityRecord out;
  out.ref = recR.value->ref;
  out.sandbox = normalized;
  return referee::Result<SandboxIdentityRecord>::ok(std::move(out));
}

referee::Result<std::optional<SandboxIdentityRecord>> CapabilityContextStore::get_sandbox(
    referee::ObjectID id) {
  auto recR = store_.get_latest(id);
  if (!recR) return referee::Result<std::optional<SandboxIdentityRecord>>::err(recR.error.value());
  if (!recR.value->has_value()) {
    return referee::Result<std::optional<SandboxIdentityRecord>>::ok(std::nullopt);
  }
  if (recR.value->value().type != kSandboxIdentityType) {
    return referee::Result<std::optional<SandboxIdentityRecord>>::err(
        referee::ErrorCode::InvalidArgument,
        "object is not a sandbox identity");
  }

  auto recordR = sandbox_record_from_object(recR.value->value());
  if (!recordR) {
    return referee::Result<std::optional<SandboxIdentityRecord>>::err(recordR.error.value());
  }
  return referee::Result<std::optional<SandboxIdentityRecord>>::ok(recordR.value.value());
}

referee::Result<std::vector<SandboxIdentityRecord>> CapabilityContextStore::list_sandboxes() {
  auto recordsR = store_.list_by_type(kSandboxIdentityType);
  if (!recordsR) return referee::Result<std::vector<SandboxIdentityRecord>>::err(recordsR.error.value());

  std::vector<SandboxIdentityRecord> out;
  out.reserve(recordsR.value->size());
  for (const auto& rec : recordsR.value.value()) {
    auto sandboxR = sandbox_record_from_object(rec);
    if (!sandboxR) {
      return referee::Result<std::vector<SandboxIdentityRecord>>::err(sandboxR.error.value());
    }
    out.push_back(std::move(sandboxR.value.value()));
  }

  std::sort(out.begin(), out.end(), [](const SandboxIdentityRecord& lhs,
                                       const SandboxIdentityRecord& rhs) {
    return lhs.sandbox.id.to_hex() < rhs.sandbox.id.to_hex();
  });
  return referee::Result<std::vector<SandboxIdentityRecord>>::ok(std::move(out));
}

referee::Result<std::vector<SandboxIdentityRecord>>
CapabilityContextStore::list_sandboxes_for_subject(referee::ObjectID subject) {
  auto sandboxesR = list_sandboxes();
  if (!sandboxesR) return sandboxesR;

  std::vector<SandboxIdentityRecord> out;
  for (auto& record : sandboxesR.value.value()) {
    auto it = std::find(record.sandbox.subjects.begin(),
                        record.sandbox.subjects.end(),
                        subject);
    if (it != record.sandbox.subjects.end()) out.push_back(std::move(record));
  }
  return referee::Result<std::vector<SandboxIdentityRecord>>::ok(std::move(out));
}

referee::Result<CapabilityContextRecord> CapabilityContextStore::persist_context(
    const CapabilityContext& context) {
  auto normalizedR = validate_context(context);
  if (!normalizedR) {
    return referee::Result<CapabilityContextRecord>::err(normalizedR.error.value());
  }

  const auto& normalized = normalizedR.value.value();
  auto payload = nlohmann::json::to_cbor(context_to_json(normalized));
  auto recR = store_.create_object_with_id(normalized.id,
                                           kCapabilityContextType,
                                           referee::ObjectID{},
                                           payload);
  if (!recR) return referee::Result<CapabilityContextRecord>::err(recR.error.value());

  CapabilityContextRecord out;
  out.ref = recR.value->ref;
  out.context = normalized;
  return referee::Result<CapabilityContextRecord>::ok(std::move(out));
}

referee::Result<std::optional<CapabilityContextRecord>> CapabilityContextStore::get_context(
    referee::ObjectID id) {
  auto recR = store_.get_latest(id);
  if (!recR) return referee::Result<std::optional<CapabilityContextRecord>>::err(recR.error.value());
  if (!recR.value->has_value()) {
    return referee::Result<std::optional<CapabilityContextRecord>>::ok(std::nullopt);
  }
  if (recR.value->value().type != kCapabilityContextType) {
    return referee::Result<std::optional<CapabilityContextRecord>>::err(
        referee::ErrorCode::InvalidArgument,
        "object is not a capability context");
  }

  auto recordR = record_from_object(recR.value->value());
  if (!recordR) {
    return referee::Result<std::optional<CapabilityContextRecord>>::err(recordR.error.value());
  }
  return referee::Result<std::optional<CapabilityContextRecord>>::ok(recordR.value.value());
}

referee::Result<std::vector<CapabilityContextRecord>> CapabilityContextStore::list_contexts() {
  auto recordsR = store_.list_by_type(kCapabilityContextType);
  if (!recordsR) return referee::Result<std::vector<CapabilityContextRecord>>::err(recordsR.error.value());

  std::vector<CapabilityContextRecord> out;
  out.reserve(recordsR.value->size());
  for (const auto& rec : recordsR.value.value()) {
    auto contextR = record_from_object(rec);
    if (!contextR) {
      return referee::Result<std::vector<CapabilityContextRecord>>::err(contextR.error.value());
    }
    out.push_back(std::move(contextR.value.value()));
  }

  std::sort(out.begin(), out.end(), [](const CapabilityContextRecord& lhs,
                                       const CapabilityContextRecord& rhs) {
    return lhs.context.id.to_hex() < rhs.context.id.to_hex();
  });
  return referee::Result<std::vector<CapabilityContextRecord>>::ok(std::move(out));
}

referee::Result<std::vector<CapabilityContextRecord>>
CapabilityContextStore::list_contexts_for_subject(referee::ObjectID subject) {
  auto contextsR = list_contexts();
  if (!contextsR) return contextsR;

  std::vector<CapabilityContextRecord> out;
  for (auto& record : contextsR.value.value()) {
    if (record.context.subject == subject) out.push_back(std::move(record));
  }
  return referee::Result<std::vector<CapabilityContextRecord>>::ok(std::move(out));
}

} // namespace iris::service
