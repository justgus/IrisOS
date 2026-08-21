#pragma once

#include "machine/scalars.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace iris::machine {

constexpr referee::TypeID kBlobType{0x4D4143480000000BULL};
constexpr referee::TypeID kSpanType{0x4D4143480000000CULL};
constexpr referee::TypeID kSliceType{0x4D4143480000000DULL};
constexpr referee::TypeID kPacketType{0x4D4143480000000EULL};

class Blob {
public:
  Blob();
  explicit Blob(std::vector<Byte> bytes);

  std::span<const Byte> bytes() const { return *storage_; }
  std::size_t size() const { return storage_->size(); }
  bool empty() const { return storage_->empty(); }

  friend bool operator==(const Blob& left, const Blob& right) {
    return *left.storage_ == *right.storage_;
  }

private:
  friend class Slice;
  std::shared_ptr<const std::vector<Byte>> storage_;
};

class Span {
public:
  static referee::Result<Span> create(std::size_t offset, std::size_t length);

  std::size_t offset() const { return offset_; }
  std::size_t length() const { return length_; }
  std::size_t end() const { return offset_ + length_; }

  friend bool operator==(Span, Span) = default;

private:
  Span(std::size_t offset, std::size_t length) : offset_(offset), length_(length) {}

  std::size_t offset_{};
  std::size_t length_{};
};

class Slice {
public:
  static referee::Result<Slice> create(const Blob& blob, Span span);

  std::span<const Byte> bytes() const {
    return std::span<const Byte>(*storage_).subspan(offset_, length_);
  }
  std::size_t size() const { return length_; }
  bool empty() const { return length_ == 0; }

  friend bool operator==(const Slice& left, const Slice& right) {
    return left.size() == right.size()
        && std::equal(left.bytes().begin(), left.bytes().end(), right.bytes().begin());
  }

private:
  Slice(std::shared_ptr<const std::vector<Byte>> storage,
        std::size_t offset,
        std::size_t length)
      : storage_(std::move(storage)), offset_(offset), length_(length) {}

  std::shared_ptr<const std::vector<Byte>> storage_;
  std::size_t offset_{};
  std::size_t length_{};
};

class Packet {
public:
  Packet() = default;
  explicit Packet(Blob payload) : payload_(std::move(payload)) {}

  const Blob& payload() const { return payload_; }
  std::size_t size() const { return payload_.size(); }
  bool empty() const { return payload_.empty(); }

  friend bool operator==(const Packet&, const Packet&) = default;

private:
  Blob payload_{};
};

} // namespace iris::machine
