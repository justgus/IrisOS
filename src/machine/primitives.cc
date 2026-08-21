#include "machine/scalars.h"

#include <utility>

namespace iris::machine {

referee::Result<Bit> Bit::create(std::uint8_t value) {
  if (value > 1) {
    return referee::Result<Bit>::err(referee::ErrorCode::InvalidArgument,
                                     "bit value must be zero or one");
  }
  return referee::Result<Bit>::ok(Bit(value));
}

referee::Result<Nibble> Nibble::create(std::uint8_t value) {
  if (value > 0x0F) {
    return referee::Result<Nibble>::err(referee::ErrorCode::InvalidArgument,
                                        "nibble value must fit in four bits");
  }
  return referee::Result<Nibble>::ok(Nibble(value));
}

UInt128::UInt128(std::uint64_t value) {
  for (std::size_t i = 0; i < sizeof(value); ++i) {
    bytes_[bytes_.size() - 1 - i] = static_cast<std::uint8_t>(value & 0xFFU);
    value >>= 8U;
  }
}

bool UInt128::fits(std::uint8_t bit_width) const {
  if (bit_width == 0 || bit_width > 128) return false;

  const auto unused_bits = static_cast<std::uint8_t>(128 - bit_width);
  const auto whole_bytes = static_cast<std::size_t>(unused_bits / 8);
  const auto partial_bits = static_cast<std::uint8_t>(unused_bits % 8);

  for (std::size_t i = 0; i < whole_bytes; ++i) {
    if (bytes_[i] != 0) return false;
  }
  if (partial_bits == 0) return true;

  const auto mask = static_cast<std::uint8_t>(0xFFU << (8U - partial_bits));
  return (bytes_[whole_bytes] & mask) == 0;
}

referee::Result<Alignment> Alignment::create(std::uint64_t bytes) {
  if (bytes == 0 || (bytes & (bytes - 1)) != 0) {
    return referee::Result<Alignment>::err(referee::ErrorCode::InvalidArgument,
                                           "alignment must be a nonzero power of two");
  }
  return referee::Result<Alignment>::ok(Alignment(bytes));
}

referee::Result<Address> Address::create(UInt128 value, std::uint8_t bit_width) {
  if (bit_width == 0 || bit_width > 128) {
    return referee::Result<Address>::err(referee::ErrorCode::InvalidArgument,
                                         "address width must be between 1 and 128 bits");
  }
  if (!value.fits(bit_width)) {
    return referee::Result<Address>::err(referee::ErrorCode::InvalidArgument,
                                         "address value does not fit its declared width");
  }
  return referee::Result<Address>::ok(Address(std::move(value), bit_width));
}

} // namespace iris::machine
