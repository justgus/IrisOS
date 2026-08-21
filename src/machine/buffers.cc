#include "machine/buffers.h"

#include <limits>
#include <utility>

namespace iris::machine {

Blob::Blob() : storage_(std::make_shared<const std::vector<Byte>>()) {}

Blob::Blob(std::vector<Byte> bytes)
    : storage_(std::make_shared<const std::vector<Byte>>(std::move(bytes))) {}

referee::Result<Span> Span::create(std::size_t offset, std::size_t length) {
  if (length > std::numeric_limits<std::size_t>::max() - offset) {
    return referee::Result<Span>::err(referee::ErrorCode::InvalidArgument,
                                      "span end overflows size_t");
  }
  return referee::Result<Span>::ok(Span(offset, length));
}

referee::Result<Slice> Slice::create(const Blob& blob, Span span) {
  if (span.offset() > blob.size() || span.length() > blob.size() - span.offset()) {
    return referee::Result<Slice>::err(referee::ErrorCode::InvalidArgument,
                                       "slice range is outside the blob");
  }
  return referee::Result<Slice>::ok(Slice(blob.storage_, span.offset(), span.length()));
}

} // namespace iris::machine
