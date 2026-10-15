#include "services/allocation_service.h"

#include "services/request_validation.h"

#include <limits>
#include <new>
#include <stdexcept>

namespace iris::service {

AllocationService::AllocationService(referee::ObjectID id, AllocationLimits limits)
    : desc_{id, kAllocationServiceType, "allocation", {},
            {{"allocation.allocate", kAllocationAllocateType, {}},
             {"allocation.inspect", kAllocationInspectType, {}},
             {"allocation.release", kAllocationReleaseType, {}}}},
      limits_(limits) {}

referee::Result<MessageEnvelope> AllocationService::handle_message(const MessageEnvelope& request) {
  try {
    auto operation = detail::operation_for(desc_, request);
    if (!operation) return referee::Result<MessageEnvelope>::err(*operation.error);
    auto payload = detail::object_payload(request.payload_cbor);
    if (!payload) return referee::Result<MessageEnvelope>::err(*payload.error);
    const auto& input = *payload.value;
    if (input.size() != 1) throw std::invalid_argument("expected exactly one argument");
    if (*operation.value == kAllocationAllocateType) {
      const auto size = detail::positive_size(input.at("size"));
      if (size > std::numeric_limits<std::size_t>::max() ||
          size > referee::Bytes{}.max_size()) {
        throw std::invalid_argument("allocation size is not representable");
      }
      std::uint64_t subject_bytes = 0;
      for (const auto& [id, allocation] : allocations_) {
        (void)id;
        if (allocation.owner == request.sender) subject_bytes += allocation.bytes.size();
      }
      if (allocations_.size() >= limits_.allocation_count ||
          size > limits_.total_bytes - total_bytes_ ||
          size > limits_.bytes_per_subject ||
          subject_bytes > limits_.bytes_per_subject - size) {
        return referee::Result<MessageEnvelope>::err(referee::ErrorCode::FailedPrecondition,
                                                     "allocation limit exceeded");
      }
      auto id = referee::ObjectID::random().to_hex();
      while (allocations_.contains(id)) id = referee::ObjectID::random().to_hex();
      auto response = make_response(request, desc_.id, kAllocationResponseType,
          nlohmann::json::to_cbor({{"id", id}, {"size", size},
                                   {"owner", request.sender.to_hex()}}));
      // Construct buffer and response before publishing; failed allocation leaves quotas unchanged.
      Allocation allocation{request.sender, referee::Bytes(static_cast<std::size_t>(size), 0)};
      allocations_.emplace(id, std::move(allocation));
      total_bytes_ += size;
      return referee::Result<MessageEnvelope>::ok(std::move(response));
    }
    const auto id = referee::ObjectID::from_hex(input.at("id").get<std::string>()).to_hex();
    auto found = allocations_.find(id);
    if (found == allocations_.end()) {
      return referee::Result<MessageEnvelope>::err(referee::ErrorCode::NotFound,
                                                   "allocation not found");
    }
    if (found->second.owner != request.sender) {
      return referee::Result<MessageEnvelope>::err(referee::ErrorCode::FailedPrecondition,
                                                   "allocation owner mismatch");
    }
    auto response = make_response(request, desc_.id, kAllocationResponseType,
        nlohmann::json::to_cbor({{"id", id}, {"size", found->second.bytes.size()},
                                 {"owner", found->second.owner.to_hex()}}));
    if (*operation.value == kAllocationReleaseType) {
      total_bytes_ -= found->second.bytes.size();
      allocations_.erase(found);
    }
    return referee::Result<MessageEnvelope>::ok(std::move(response));
  } catch (const std::bad_alloc&) {
    return referee::Result<MessageEnvelope>::err(referee::ErrorCode::IoError,
                                                 "allocation failed");
  } catch (const std::exception& ex) {
    return referee::Result<MessageEnvelope>::err(referee::ErrorCode::InvalidArgument, ex.what());
  }
}

} // namespace iris::service
