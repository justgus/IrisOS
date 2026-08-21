#pragma once

#include "referee/referee.h"

#include <array>
#include <cstdint>
#include <type_traits>

namespace iris::machine {

constexpr referee::TypeID kBitType{0x4D41434800000001ULL};
constexpr referee::TypeID kNibbleType{0x4D41434800000002ULL};
constexpr referee::TypeID kByteType{0x4D41434800000003ULL};
constexpr referee::TypeID kWord16Type{0x4D41434800000004ULL};
constexpr referee::TypeID kWord32Type{0x4D41434800000005ULL};
constexpr referee::TypeID kWord64Type{0x4D41434800000006ULL};
constexpr referee::TypeID kUInt128Type{0x4D41434800000007ULL};
constexpr referee::TypeID kByteOrderType{0x4D41434800000008ULL};
constexpr referee::TypeID kAlignmentType{0x4D41434800000009ULL};
constexpr referee::TypeID kAddressType{0x4D4143480000000AULL};

class Bit {
public:
  static referee::Result<Bit> create(std::uint8_t value);
  std::uint8_t value() const { return value_; }

  friend bool operator==(Bit, Bit) = default;

private:
  explicit Bit(std::uint8_t value) : value_(value) {}
  std::uint8_t value_{};
};

class Nibble {
public:
  static referee::Result<Nibble> create(std::uint8_t value);
  std::uint8_t value() const { return value_; }

  friend bool operator==(Nibble, Nibble) = default;

private:
  explicit Nibble(std::uint8_t value) : value_(value) {}
  std::uint8_t value_{};
};

class Byte {
public:
  explicit constexpr Byte(std::uint8_t value = 0) : value_(value) {}
  constexpr std::uint8_t value() const { return value_; }

  friend bool operator==(Byte, Byte) = default;

private:
  std::uint8_t value_{};
};

template <typename Storage>
class Word {
  static_assert(std::is_same_v<Storage, std::uint16_t>
                || std::is_same_v<Storage, std::uint32_t>
                || std::is_same_v<Storage, std::uint64_t>);

public:
  explicit constexpr Word(Storage value = 0) : value_(value) {}
  constexpr Storage value() const { return value_; }

  friend bool operator==(Word, Word) = default;

private:
  Storage value_{};
};

using Word16 = Word<std::uint16_t>;
using Word32 = Word<std::uint32_t>;
using Word64 = Word<std::uint64_t>;

class UInt128 {
public:
  using Bytes = std::array<std::uint8_t, 16>;

  constexpr UInt128() = default;
  explicit constexpr UInt128(Bytes bytes) : bytes_(bytes) {}
  explicit UInt128(std::uint64_t value);

  constexpr const Bytes& bytes() const { return bytes_; }
  bool fits(std::uint8_t bit_width) const;

  friend bool operator==(const UInt128&, const UInt128&) = default;

private:
  Bytes bytes_{};
};

enum class ByteOrder {
  Little,
  Big,
  Native
};

class Alignment {
public:
  static referee::Result<Alignment> create(std::uint64_t bytes);
  std::uint64_t bytes() const { return bytes_; }

  friend bool operator==(Alignment, Alignment) = default;

private:
  explicit Alignment(std::uint64_t bytes) : bytes_(bytes) {}
  std::uint64_t bytes_{};
};

class Address {
public:
  static referee::Result<Address> create(UInt128 value, std::uint8_t bit_width);

  const UInt128& value() const { return value_; }
  std::uint8_t bit_width() const { return bit_width_; }

  friend bool operator==(const Address&, const Address&) = default;

private:
  Address(UInt128 value, std::uint8_t bit_width)
      : value_(value), bit_width_(bit_width) {}

  UInt128 value_{};
  std::uint8_t bit_width_{};
};

} // namespace iris::machine
