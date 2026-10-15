#pragma once

#include "services/capability_context.h"
#include "services/service.h"

namespace iris::service {

constexpr referee::TypeID kInspectionServiceType{0x535643494E530001ULL};
constexpr referee::TypeID kInspectionContextsType{0x535643494E531001ULL};
constexpr referee::TypeID kInspectionSandboxesType{0x535643494E531002ULL};
constexpr referee::TypeID kInspectionResponseType{0x535643494E532001ULL};

// Read-only local service. Context store and its backing Referee store must outlive the binding.
// Uses serialized store calls and trusted envelope sender identity, like ServiceHostManager.
class InspectionService final : public ServiceObject {
public:
  InspectionService(referee::ObjectID id, CapabilityContextStore& contexts);

  ServiceDescriptor descriptor() const override { return desc_; }
  referee::Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override;

private:
  ServiceDescriptor desc_;
  CapabilityContextStore& contexts_;
};

} // namespace iris::service
