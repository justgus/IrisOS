#pragma once

#include "services/service.h"

#include <map>

namespace iris::service {

constexpr referee::TypeID kAllocationServiceType{0x535643414C4C0001ULL};
constexpr referee::TypeID kAllocationAllocateType{0x535643414C4C1001ULL};
constexpr referee::TypeID kAllocationInspectType{0x535643414C4C1002ULL};
constexpr referee::TypeID kAllocationReleaseType{0x535643414C4C1003ULL};
constexpr referee::TypeID kAllocationResponseType{0x535643414C4C2001ULL};

struct AllocationLimits {
  std::uint64_t total_bytes;
  std::uint64_t bytes_per_subject;
  std::size_t allocation_count;
};

// Local serialized handler. Buffers belong to this instance and are not persisted.
// Descriptors can recover independently; rebinding a new instance starts empty.
class AllocationService final : public ServiceObject {
public:
  AllocationService(referee::ObjectID id, AllocationLimits limits);
  AllocationService(const AllocationService&) = delete;
  AllocationService& operator=(const AllocationService&) = delete;

  ServiceDescriptor descriptor() const override { return desc_; }
  referee::Result<MessageEnvelope> handle_message(const MessageEnvelope& request) override;

private:
  struct Allocation {
    referee::ObjectID owner;
    referee::Bytes bytes;
  };
  ServiceDescriptor desc_;
  AllocationLimits limits_;
  std::uint64_t total_bytes_{0};
  std::map<std::string, Allocation> allocations_;
};

} // namespace iris::service
