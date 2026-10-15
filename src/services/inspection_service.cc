#include "services/inspection_service.h"

#include "services/request_validation.h"

#include <new>

namespace iris::service {

InspectionService::InspectionService(referee::ObjectID id, CapabilityContextStore& contexts)
    : desc_{id, kInspectionServiceType, "inspection", {},
            {{"inspection.contexts", kInspectionContextsType, {}},
             {"inspection.sandboxes", kInspectionSandboxesType, {}}}},
      contexts_(contexts) {}

referee::Result<MessageEnvelope> InspectionService::handle_message(const MessageEnvelope& request) {
  try {
    auto operation = detail::operation_for(desc_, request);
    if (!operation) return referee::Result<MessageEnvelope>::err(*operation.error);
    auto payload = detail::object_payload(request.payload_cbor);
    if (!payload) return referee::Result<MessageEnvelope>::err(*payload.error);
    if (!payload.value->empty()) {
      return referee::Result<MessageEnvelope>::err(referee::ErrorCode::InvalidArgument,
                                                   "inspection accepts no subject overrides or arguments");
    }
    nlohmann::json output;
    if (*operation.value == kInspectionContextsType) {
      auto contexts = contexts_.list_contexts_for_subject(request.sender);
      if (!contexts) return referee::Result<MessageEnvelope>::err(*contexts.error);
      output["contexts"] = nlohmann::json::array();
      for (const auto& record : *contexts.value) {
        const auto& context = record.context;
        nlohmann::json item{{"id", context.id.to_hex()}, {"subject", context.subject.to_hex()}};
        item["sandbox"] = context.sandbox ? nlohmann::json(context.sandbox->to_hex()) : nlohmann::json(nullptr);
        item["grants"] = nlohmann::json::array();
        for (const auto& grant : context.grants) item["grants"].push_back(grant.name);
        output["contexts"].push_back(std::move(item));
      }
    } else {
      auto sandboxes = contexts_.list_sandboxes_for_subject(request.sender);
      if (!sandboxes) return referee::Result<MessageEnvelope>::err(*sandboxes.error);
      output["sandboxes"] = nlohmann::json::array();
      for (const auto& record : *sandboxes.value) {
        // Membership is caller-filtered; never serialize the sandbox's other subjects.
        output["sandboxes"].push_back({{"id", record.sandbox.id.to_hex()},
                                       {"name", record.sandbox.name},
                                       {"subjects", {request.sender.to_hex()}}});
      }
    }
    return referee::Result<MessageEnvelope>::ok(make_response(
        request, desc_.id, kInspectionResponseType, nlohmann::json::to_cbor(output)));
  } catch (const std::bad_alloc&) {
    return referee::Result<MessageEnvelope>::err(referee::ErrorCode::IoError,
                                                 "inspection allocation failed");
  } catch (const std::exception& ex) {
    return referee::Result<MessageEnvelope>::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
}

} // namespace iris::service
