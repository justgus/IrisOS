#include "refract/bootstrap.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <string_view>

#include <nlohmann/json.hpp>

namespace iris::refract {

namespace {

referee::ObjectID active_caliper_catalog_id() {
  referee::ObjectID id{};
  constexpr std::array<std::uint8_t, 16> bytes = {
    'C', 'A', 'L', 'I', 'P', 'E', 'R', '-', 'B', 'A', 'S', 'E', '-', '0', '0', '2'
  };
  id.bytes = bytes;
  return id;
}

struct Rational {
  std::int64_t numerator{0};
  std::int64_t denominator{1};
};

std::uint64_t magnitude(std::int64_t value) {
  if (value >= 0) return static_cast<std::uint64_t>(value);
  return static_cast<std::uint64_t>(-(value + 1)) + 1;
}

bool checked_add(std::int64_t left, std::int64_t right, std::int64_t* result) {
  constexpr auto min = std::numeric_limits<std::int64_t>::min();
  constexpr auto max = std::numeric_limits<std::int64_t>::max();
  if ((right > 0 && left > max - right) || (right < 0 && left < min - right)) return false;
  *result = left + right;
  return true;
}

bool checked_multiply(std::int64_t left, std::int64_t right, std::int64_t* result) {
  constexpr auto min = std::numeric_limits<std::int64_t>::min();
  constexpr auto max = std::numeric_limits<std::int64_t>::max();
  if (left == 0 || right == 0) {
    *result = 0;
    return true;
  }
  if ((left == -1 && right == min) || (right == -1 && left == min)) return false;
  if (left > 0) {
    if ((right > 0 && left > max / right) || (right < 0 && right < min / left)) return false;
  } else {
    if ((right > 0 && left < min / right) || (right < 0 && left < max / right)) return false;
  }
  *result = left * right;
  return true;
}

referee::Result<Rational> normalize_rational(std::int64_t numerator, std::int64_t denominator) {
  if (denominator == 0) return referee::Result<Rational>::err("zero denominator in exact Caliper conversion");
  if (denominator < 0) {
    if (denominator == std::numeric_limits<std::int64_t>::min()
        || numerator == std::numeric_limits<std::int64_t>::min()) {
      return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
    }
    denominator = -denominator;
    numerator = -numerator;
  }
  const auto divisor = std::gcd(magnitude(numerator), static_cast<std::uint64_t>(denominator));
  numerator /= static_cast<std::int64_t>(divisor);
  denominator /= static_cast<std::int64_t>(divisor);
  return referee::Result<Rational>::ok(Rational{numerator, denominator});
}

referee::Result<Rational> parse_decimal_rational(std::string_view text) {
  if (text.empty()) return referee::Result<Rational>::err("invalid decimal value for Caliper conversion");

  bool negative = false;
  std::size_t index = 0;
  if (text[index] == '+' || text[index] == '-') {
    negative = text[index] == '-';
    ++index;
  }
  if (index == text.size()) return referee::Result<Rational>::err("invalid decimal value for Caliper conversion");

  std::string digits;
  digits.reserve(text.size());
  std::int64_t fractional_digits = 0;
  bool saw_digit = false;
  bool saw_decimal = false;
  for (; index < text.size() && text[index] != 'e' && text[index] != 'E'; ++index) {
    const char c = text[index];
    if (c == '.' && !saw_decimal) {
      saw_decimal = true;
      continue;
    }
    if (c < '0' || c > '9') return referee::Result<Rational>::err("invalid decimal value for Caliper conversion");
    saw_digit = true;
    digits.push_back(c);
    if (saw_decimal) {
      if (fractional_digits == std::numeric_limits<std::int64_t>::max()) {
        return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
      }
      ++fractional_digits;
    }
  }
  if (!saw_digit) return referee::Result<Rational>::err("invalid decimal value for Caliper conversion");

  int exponent = 0;
  if (index < text.size()) {
    ++index;
    if (index == text.size()) return referee::Result<Rational>::err("invalid decimal value for Caliper conversion");
    const auto* begin = text.data() + index;
    const auto* end = text.data() + text.size();
    if (*begin == '+') ++begin;
    if (begin == end) return referee::Result<Rational>::err("invalid decimal exponent for Caliper conversion");
    auto parsed = std::from_chars(begin, end, exponent);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      return referee::Result<Rational>::err("decimal exponent is out of range for Caliper conversion");
    }
  }

  const auto first_nonzero = digits.find_first_not_of('0');
  if (first_nonzero == std::string::npos) return referee::Result<Rational>::ok(Rational{0, 1});
  digits.erase(0, first_nonzero);
  std::int64_t trailing_zeros = 0;
  while (digits.size() > 1 && digits.back() == '0') {
    digits.pop_back();
    ++trailing_zeros;
  }
  const auto decimal_power_wide = static_cast<__int128_t>(exponent) - fractional_digits + trailing_zeros;
  if (decimal_power_wide > std::numeric_limits<std::int64_t>::max()
      || decimal_power_wide < std::numeric_limits<std::int64_t>::min()) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  const auto decimal_power = static_cast<std::int64_t>(decimal_power_wide);
  __uint128_t magnitude_value = 0;
  constexpr auto uint128_max = ~static_cast<__uint128_t>(0);
  for (const char digit : digits) {
    const auto numeric_digit = static_cast<unsigned>(digit - '0');
    if (magnitude_value > (uint128_max - numeric_digit) / 10) {
      return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
    }
    magnitude_value = magnitude_value * 10 + numeric_digit;
  }

  constexpr auto positive_limit = static_cast<__uint128_t>(std::numeric_limits<std::int64_t>::max());
  constexpr auto negative_limit = positive_limit + 1;
  const auto numerator_limit = negative ? negative_limit : positive_limit;
  if (decimal_power >= 0) {
    if (decimal_power > 19) {
      return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
    }
    for (std::int64_t i = 0; i < decimal_power; ++i) {
      if (magnitude_value > numerator_limit / 10) {
        return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
      }
      magnitude_value *= 10;
    }
    if (magnitude_value > numerator_limit) {
      return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
    }
    if (negative && magnitude_value == negative_limit) {
      return referee::Result<Rational>::ok(Rational{std::numeric_limits<std::int64_t>::min(), 1});
    }
    const auto numerator = static_cast<std::int64_t>(magnitude_value);
    return referee::Result<Rational>::ok(Rational{negative ? -numerator : numerator, 1});
  }

  auto denominator_power = -decimal_power;
  auto twos = denominator_power;
  while (twos > 0 && magnitude_value % 2 == 0) {
    magnitude_value /= 2;
    --twos;
  }
  auto fives = denominator_power;
  while (fives > 0 && magnitude_value % 5 == 0) {
    magnitude_value /= 5;
    --fives;
  }
  if (twos > 62 || fives > 27 || magnitude_value > numerator_limit) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  __uint128_t denominator_value = 1;
  for (std::int64_t i = 0; i < twos; ++i) denominator_value *= 2;
  for (std::int64_t i = 0; i < fives; ++i) denominator_value *= 5;
  if (denominator_value > positive_limit) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  if (negative && magnitude_value == negative_limit) {
    return referee::Result<Rational>::ok(Rational{
        std::numeric_limits<std::int64_t>::min(), static_cast<std::int64_t>(denominator_value)});
  }
  const auto signed_magnitude = static_cast<std::int64_t>(magnitude_value);
  const auto numerator = negative ? -signed_magnitude : signed_magnitude;
  return referee::Result<Rational>::ok(
      Rational{numerator, static_cast<std::int64_t>(denominator_value)});
}

referee::Result<Rational> rational_from_double(double value) {
  if (!std::isfinite(value)) return referee::Result<Rational>::err("non-finite Caliper conversion factor");
  std::array<char, 64> buffer{};
  auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value,
                                 std::chars_format::general);
  if (converted.ec != std::errc{}) {
    return referee::Result<Rational>::err("could not encode Caliper conversion factor");
  }
  return parse_decimal_rational(std::string_view(buffer.data(),
                                                static_cast<std::size_t>(converted.ptr - buffer.data())));
}

referee::Result<Rational> add_rational(const Rational& left, const Rational& right) {
  const auto divisor = std::gcd(static_cast<std::uint64_t>(left.denominator),
                                static_cast<std::uint64_t>(right.denominator));
  const auto left_factor = right.denominator / static_cast<std::int64_t>(divisor);
  const auto right_factor = left.denominator / static_cast<std::int64_t>(divisor);
  std::int64_t left_numerator = 0;
  std::int64_t right_numerator = 0;
  std::int64_t numerator = 0;
  std::int64_t denominator = 0;
  if (!checked_multiply(left.numerator, left_factor, &left_numerator)
      || !checked_multiply(right.numerator, right_factor, &right_numerator)
      || !checked_add(left_numerator, right_numerator, &numerator)
      || !checked_multiply(left.denominator, left_factor, &denominator)) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  return normalize_rational(numerator, denominator);
}

referee::Result<Rational> subtract_rational(const Rational& left, const Rational& right) {
  if (right.numerator == std::numeric_limits<std::int64_t>::min()) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  return add_rational(left, Rational{-right.numerator, right.denominator});
}

referee::Result<Rational> multiply_rational(const Rational& left, const Rational& right) {
  const auto cross_left = std::gcd(magnitude(left.numerator), static_cast<std::uint64_t>(right.denominator));
  const auto cross_right = std::gcd(magnitude(right.numerator), static_cast<std::uint64_t>(left.denominator));
  const auto left_numerator = left.numerator / static_cast<std::int64_t>(cross_left);
  const auto right_denominator = right.denominator / static_cast<std::int64_t>(cross_left);
  const auto right_numerator = right.numerator / static_cast<std::int64_t>(cross_right);
  const auto left_denominator = left.denominator / static_cast<std::int64_t>(cross_right);
  std::int64_t numerator = 0;
  std::int64_t denominator = 0;
  if (!checked_multiply(left_numerator, right_numerator, &numerator)
      || !checked_multiply(left_denominator, right_denominator, &denominator)) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  return normalize_rational(numerator, denominator);
}

referee::Result<Rational> divide_rational(const Rational& left, const Rational& right) {
  if (right.numerator == 0) return referee::Result<Rational>::err("zero scale in Caliper conversion");
  auto reciprocal = normalize_rational(right.denominator, right.numerator);
  if (!reciprocal) return reciprocal;
  return multiply_rational(left, *reciprocal.value);
}

referee::Result<Rational> rational_for_factor(const std::optional<double>& factor, double default_value) {
  return rational_from_double(factor.value_or(default_value));
}

referee::Result<Rational> rational_for_offset(const std::optional<double>& offset) {
  return rational_from_double(offset.value_or(0.0));
}

referee::Result<double> rational_to_double(const Rational& value) {
  if (value.numerator == 0) return referee::Result<double>::ok(0.0);

  const bool negative = value.numerator < 0;
  const auto numerator = magnitude(value.numerator);
  const auto denominator = static_cast<std::uint64_t>(value.denominator);
  int exponent = static_cast<int>(std::bit_width(numerator))
                 - static_cast<int>(std::bit_width(denominator));
  if (exponent >= 0) {
    if (static_cast<__uint128_t>(numerator)
        < (static_cast<__uint128_t>(denominator) << exponent)) {
      --exponent;
    }
  } else if ((static_cast<__uint128_t>(numerator) << -exponent) < denominator) {
    --exponent;
  }

  __uint128_t normalized_numerator = numerator;
  __uint128_t normalized_denominator = denominator;
  if (exponent >= 0) normalized_denominator <<= exponent;
  else normalized_numerator <<= -exponent;
  auto remainder = normalized_numerator - normalized_denominator;
  std::uint64_t fraction = 0;
  for (int bit = 0; bit < 52; ++bit) {
    remainder <<= 1;
    fraction <<= 1;
    if (remainder >= normalized_denominator) {
      remainder -= normalized_denominator;
      fraction |= 1;
    }
  }

  auto significand = (std::uint64_t{1} << 52) | fraction;
  const auto twice_remainder = remainder << 1;
  if (twice_remainder > normalized_denominator
      || (twice_remainder == normalized_denominator && (significand & 1) != 0)) {
    ++significand;
  }
  if (significand == (std::uint64_t{1} << 53)) {
    significand >>= 1;
    ++exponent;
  }

  // The signed 64-bit rational domain keeps every nonzero value in binary64's
  // normal range, and the rounded exponent remains representable.
  const auto exponent_bits = static_cast<std::uint64_t>(exponent + 1023);
  const auto sign_bit = negative ? (std::uint64_t{1} << 63) : 0;
  const auto bits = sign_bit | (exponent_bits << 52)
                    | (significand & ((std::uint64_t{1} << 52) - 1));
  return referee::Result<double>::ok(std::bit_cast<double>(bits));
}

const CaliperCatalogUnit* find_caliper_unit(const std::vector<CaliperCatalogUnit>& catalog,
                                            std::string_view query) {
  for (const auto& unit : catalog) {
    if (unit.symbol == query) return &unit;
  }
  for (const auto& unit : catalog) {
    if (unit.name == query) return &unit;
  }
  return nullptr;
}

struct AffineTransform {
  Rational scale{1, 1};
  Rational offset{0, 1};
};

referee::Result<AffineTransform> transform_to_root(
    const std::vector<CaliperCatalogUnit>& catalog,
    const CaliperCatalogUnit& unit) {
  AffineTransform transform;
  const CaliperCatalogUnit* current = &unit;
  std::set<std::string> visited;
  auto identity = [](const CaliperCatalogUnit& candidate) {
    return candidate.object_id ? candidate.object_id->to_hex() : candidate.symbol;
  };
  while (current->base_symbol.has_value() || current->base_object_id.has_value()) {
    if (!visited.insert(identity(*current)).second) {
      return referee::Result<AffineTransform>::err("cycle in Caliper unit conversion chain");
    }
    const CaliperCatalogUnit* parent = nullptr;
    if (current->base_object_id) {
      for (const auto& candidate : catalog) {
        if (candidate.object_id == current->base_object_id) {
          parent = &candidate;
          break;
        }
      }
    } else {
      parent = find_caliper_unit(catalog, *current->base_symbol);
    }
    if (parent == nullptr) {
      const auto base_name = current->base_symbol.value_or(
          current->base_object_id ? current->base_object_id->to_hex() : std::string("<missing>"));
      return referee::Result<AffineTransform>::err("unknown base unit in Caliper conversion chain: "
                                                   + base_name);
    }
    if (parent->dimension != unit.dimension) {
      return referee::Result<AffineTransform>::err("dimension mismatch in Caliper conversion chain");
    }
    auto step_scale = current->exact_scale
        ? normalize_rational(current->exact_scale->numerator,
                             current->exact_scale->denominator)
        : rational_for_factor(current->scale, 1.0);
    if (!step_scale) return referee::Result<AffineTransform>::err(step_scale.error->message);
    auto step_offset = current->exact_offset
        ? normalize_rational(current->exact_offset->numerator,
                             current->exact_offset->denominator)
        : rational_for_offset(current->offset);
    if (!step_offset) return referee::Result<AffineTransform>::err(step_offset.error->message);
    if (step_scale.value->numerator <= 0) {
      return referee::Result<AffineTransform>::err("Caliper conversion scale must be positive");
    }
    auto new_offset = multiply_rational(transform.offset, *step_scale.value);
    if (!new_offset) return referee::Result<AffineTransform>::err(new_offset.error->message);
    new_offset = add_rational(*new_offset.value, *step_offset.value);
    if (!new_offset) return referee::Result<AffineTransform>::err(new_offset.error->message);
    auto new_scale = multiply_rational(transform.scale, *step_scale.value);
    if (!new_scale) return referee::Result<AffineTransform>::err(new_scale.error->message);
    transform.scale = *new_scale.value;
    transform.offset = *new_offset.value;
    current = parent;
  }
  if (!visited.insert(identity(*current)).second) {
    return referee::Result<AffineTransform>::err("cycle in Caliper unit conversion chain");
  }
  auto root_scale = current->exact_scale
      ? normalize_rational(current->exact_scale->numerator,
                           current->exact_scale->denominator)
      : rational_for_factor(current->scale, 1.0);
  if (!root_scale) return referee::Result<AffineTransform>::err(root_scale.error->message);
  if (root_scale.value->numerator <= 0) {
    return referee::Result<AffineTransform>::err("Caliper conversion scale must be positive");
  }
  auto root_offset = current->exact_offset
      ? normalize_rational(current->exact_offset->numerator,
                           current->exact_offset->denominator)
      : rational_for_offset(current->offset);
  if (!root_offset) return referee::Result<AffineTransform>::err(root_offset.error->message);
  return referee::Result<AffineTransform>::ok(transform);
}

} // namespace

referee::Result<std::vector<CaliperCatalogUnit>> load_caliper_catalog(
    SchemaRegistry& registry,
    referee::SqliteStore& store) {
  auto types = registry.list_types();
  if (!types) return referee::Result<std::vector<CaliperCatalogUnit>>::err(types.error->message);
  std::optional<referee::TypeID> catalog_type;
  std::optional<referee::TypeID> unit_type;
  std::optional<referee::TypeID> dimension_type;
  for (const auto& type : *types.value) {
    if (type.namespace_name != "Caliper") continue;
    if (type.name == "Catalog") catalog_type = type.type_id;
    if (type.name == "Unit") unit_type = type.type_id;
    if (type.name == "Dimension") dimension_type = type.type_id;
  }
  if (!catalog_type || !unit_type || !dimension_type) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("Caliper catalog schemas are not registered");
  }

  auto catalog_record = store.get_latest(active_caliper_catalog_id());
  if (!catalog_record) {
    if (catalog_record.error->code == referee::ErrorCode::NotFound) {
      return referee::Result<std::vector<CaliperCatalogUnit>>::err("Caliper v2 catalog is missing");
    }
    return referee::Result<std::vector<CaliperCatalogUnit>>::err(catalog_record.error.value());
  }
  if (catalog_record.value->type != *catalog_type) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("Caliper v2 catalog has the wrong object type");
  }
  auto catalog_definition = registry.get_definition_by_id(catalog_record.value->definition_id);
  if (!catalog_definition || catalog_definition.value->definition.type_id != *catalog_type) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err(
        "Caliper v2 catalog has an invalid definition reference");
  }

  auto unit_records = store.list_by_type(*unit_type);
  if (!unit_records) return referee::Result<std::vector<CaliperCatalogUnit>>::err(unit_records.error.value());
  auto dimension_records = store.list_by_type(*dimension_type);
  if (!dimension_records) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err(dimension_records.error.value());
  }
  std::map<std::string, referee::ObjectRecord> units_by_id;
  std::map<std::string, std::uint32_t> unit_schema_versions;
  std::map<std::string, std::string> dimensions_by_id;
  std::map<std::string, nlohmann::json> dimension_payloads_by_id;
  std::map<std::string, referee::Version> dimension_versions;
  std::set<std::string> all_dimension_names;
  try {
    for (const auto& record : *unit_records.value) {
      auto definition = registry.get_definition_by_id(record.definition_id);
      if (!definition || definition.value->definition.type_id != *unit_type
          || (definition.value->definition.version != 1 && definition.value->definition.version != 2)) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "Caliper Unit has an invalid definition reference");
      }
      auto existing = units_by_id.find(record.ref.id.to_hex());
      if (existing == units_by_id.end()) {
        units_by_id.emplace(record.ref.id.to_hex(), record);
        unit_schema_versions.emplace(record.ref.id.to_hex(), definition.value->definition.version);
      } else if (existing->second.ref.ver.v < record.ref.ver.v) {
        existing->second = record;
        unit_schema_versions[record.ref.id.to_hex()] = definition.value->definition.version;
      }
    }
    for (const auto& record : *dimension_records.value) {
      auto definition = registry.get_definition_by_id(record.definition_id);
      if (!definition || definition.value->definition.type_id != *dimension_type
          || definition.value->definition.version != 1) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "Caliper Dimension has an invalid definition reference");
      }
      const auto dimension = nlohmann::json::from_cbor(record.payload_cbor);
      const auto name = dimension.at("name").get<std::string>();
      if (name.empty()) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "invalid or duplicate Caliper dimension record");
      }
      auto existing = dimensions_by_id.find(record.ref.id.to_hex());
      if (existing == dimensions_by_id.end()) {
        dimensions_by_id.emplace(record.ref.id.to_hex(), name);
        dimension_payloads_by_id.emplace(record.ref.id.to_hex(), dimension);
        dimension_versions.emplace(record.ref.id.to_hex(), record.ref.ver);
      } else if (dimension_versions.at(record.ref.id.to_hex()).v < record.ref.ver.v) {
        existing->second = name;
        dimension_payloads_by_id[record.ref.id.to_hex()] = dimension;
        dimension_versions[record.ref.id.to_hex()] = record.ref.ver;
      }
    }
    for (const auto& [id, name] : dimensions_by_id) {
      (void)id;
      if (!all_dimension_names.insert(name).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "invalid or duplicate Caliper dimension record");
      }
    }
  } catch (const std::exception&) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid persisted Caliper reference target");
  }

  try {
    const auto payload = nlohmann::json::from_cbor(catalog_record.value->payload_cbor);
    if (payload.at("catalog_version").get<std::uint64_t>() != 2) {
      return referee::Result<std::vector<CaliperCatalogUnit>>::err("unsupported Caliper base catalog version");
    }
    const auto& dimensions = payload.at("dimensions");
    const auto& units = payload.at("units");
    const auto& prefixes = payload.at("prefixes");
    if (!dimensions.is_array() || !units.is_array() || !prefixes.is_array()) {
      return referee::Result<std::vector<CaliperCatalogUnit>>::err("malformed Caliper base catalog");
    }
    static const std::map<std::string, int> expected_prefixes = {
      { "Q", 30 }, { "R", 27 }, { "Y", 24 }, { "Z", 21 }, { "E", 18 }, { "P", 15 },
      { "T", 12 }, { "G", 9 }, { "M", 6 }, { "k", 3 }, { "h", 2 }, { "da", 1 },
      { "d", -1 }, { "c", -2 }, { "m", -3 }, { "µ", -6 }, { "n", -9 }, { "p", -12 },
      { "f", -15 }, { "a", -18 }, { "z", -21 }, { "y", -24 }, { "r", -27 }, { "q", -30 },
    };
    const std::map<std::string, std::string> expected_prefix_names = {
      { "Q", "quetta" }, { "R", "ronna" }, { "Y", "yotta" }, { "Z", "zetta" },
      { "E", "exa" }, { "P", "peta" }, { "T", "tera" }, { "G", "giga" },
      { "M", "mega" }, { "k", "kilo" }, { "h", "hecto" }, { "da", "deca" },
      { "d", "deci" }, { "c", "centi" }, { "m", "milli" }, { "µ", "micro" },
      { "n", "nano" }, { "p", "pico" }, { "f", "femto" }, { "a", "atto" },
      { "z", "zepto" }, { "y", "yocto" }, { "r", "ronto" }, { "q", "quecto" },
    };
    std::map<std::string, int> prefix_exponents;
    for (const auto& prefix : prefixes) {
      const auto symbol = prefix.at("symbol").get<std::string>();
      const auto name = prefix.at("name").get<std::string>();
      const auto exponent = prefix.at("exponent").get<int>();
      auto expected = expected_prefixes.find(symbol);
      if (expected == expected_prefixes.end() || expected->second != exponent
          || expected_prefix_names.at(symbol) != name
          || !prefix_exponents.emplace(symbol, exponent).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid or duplicate SI prefix metadata");
      }
    }
    if (prefix_exponents.size() != expected_prefixes.size()) {
      return referee::Result<std::vector<CaliperCatalogUnit>>::err("incomplete SI prefix metadata");
    }
    std::set<std::string> dimension_names;
    std::set<std::string> catalog_dimension_ids;
    std::map<std::string, CaliperRational> minimum_by_dimension_id;
    std::map<std::string, std::pair<CaliperRational, CaliperRational>> exact_transforms_by_unit_id;
    for (const auto& dimension : dimensions) {
      const auto name = dimension.at("name").get<std::string>();
      const auto id = dimension.at("dimension_id").get<std::string>();
      auto persisted = dimensions_by_id.find(id);
      if (name.empty() || persisted == dimensions_by_id.end() || persisted->second != name
          || !dimension_names.insert(name).second || !catalog_dimension_ids.insert(id).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid or duplicate Caliper dimension");
      }
      const auto& stored = dimension_payloads_by_id.at(id);
      if (dimension.contains("components") != stored.contains("components")
          || (dimension.contains("components") && dimension.at("components") != stored.at("components"))
          || (dimension.contains("symbol") && stored.value("symbol", std::string{})
                                                   != dimension.at("symbol").get<std::string>())) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("Caliper base Dimension snapshot mismatch");
      }
      if (dimension.contains("constraints")) {
        if (name != "Temperature" || !dimension.at("constraints").is_array()
            || dimension.at("constraints").size() != 1) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "invalid Caliper dimension constraint metadata");
        }
        const auto& constraint = dimension.at("constraints").front();
        const auto canonical_unit_id = constraint.value("unit_id", std::string{});
        const auto canonical = units_by_id.find(canonical_unit_id);
        const bool canonical_is_catalog_unit = std::any_of(
            units.begin(), units.end(), [&](const nlohmann::json& unit) {
              return unit.value("unit_id", std::string{}) == canonical_unit_id;
            });
        if (constraint.value("kind", std::string{}) != "minimum"
            || constraint.value("scope", std::string{}) != "absolute_point"
            || constraint.value("value", std::string{}) != "0"
            || canonical == units_by_id.end() || !canonical_is_catalog_unit) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "invalid Caliper dimension constraint metadata");
        }
        try {
          const auto canonical_payload = nlohmann::json::from_cbor(canonical->second.payload_cbor);
          if (canonical_payload.value("symbol", std::string{}) != "K"
              || canonical_payload.value("dimension_id", std::string{}) != id
              || canonical_payload.contains("base_unit_id")
              || canonical_payload.value("scale", 1.0) != 1.0
              || canonical_payload.value("offset", 0.0) != 0.0) {
            return referee::Result<std::vector<CaliperCatalogUnit>>::err(
                "invalid Caliper constraint canonical Unit");
          }
        } catch (const std::exception&) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "invalid Caliper constraint canonical Unit");
        }
        if (!minimum_by_dimension_id.emplace(id, CaliperRational{0, 1}).second) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "duplicate Caliper dimension constraint metadata");
        }
      }
      if (dimension.contains("exact_transforms")) {
        if (name != "Temperature" || !dimension.at("exact_transforms").is_array()
            || dimension.at("exact_transforms").size() != 1) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "invalid Caliper constraint transform metadata");
        }
        const auto& transform = dimension.at("exact_transforms").front();
        const auto unit_id = transform.value("unit_id", std::string{});
        const auto unit_record = units_by_id.find(unit_id);
        try {
          const CaliperRational scale{
              transform.at("scale").at("numerator").get<std::int64_t>(),
              transform.at("scale").at("denominator").get<std::int64_t>()};
          const CaliperRational offset{
              transform.at("offset").at("numerator").get<std::int64_t>(),
              transform.at("offset").at("denominator").get<std::int64_t>()};
          if (unit_record == units_by_id.end() || scale.denominator <= 0
              || scale.numerator <= 0 || offset.denominator <= 0
              || !exact_transforms_by_unit_id.emplace(
                  unit_id, std::pair{scale, offset}).second) {
            return referee::Result<std::vector<CaliperCatalogUnit>>::err(
                "invalid Caliper constraint transform metadata");
          }
          const auto payload = nlohmann::json::from_cbor(unit_record->second.payload_cbor);
          if (payload.value("symbol", std::string{}) != "°F"
              || payload.value("dimension_id", std::string{}) != id) {
            return referee::Result<std::vector<CaliperCatalogUnit>>::err(
                "invalid Caliper constraint transform Unit");
          }
        } catch (const std::exception&) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "invalid Caliper constraint transform metadata");
        }
      }
      if (name == "Temperature"
          && (!minimum_by_dimension_id.contains(id)
              || exact_transforms_by_unit_id.empty())) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "incomplete Caliper temperature constraint metadata");
      }
    }

    std::map<std::string, CaliperCatalogUnit> by_symbol;
    std::set<std::string> names;
    std::set<std::string> base_unit_ids;
    for (const auto& item : units) {
      const auto unit_id = item.at("unit_id").get<std::string>();
      if (unit_id.empty() || !base_unit_ids.insert(unit_id).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "duplicate or invalid Caliper base Unit reference");
      }
    }
    for (const auto& [unit_id, transform] : exact_transforms_by_unit_id) {
      (void)transform;
      if (!base_unit_ids.contains(unit_id)) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "Caliper constraint transform references a non-catalog Unit");
      }
    }
    for (const auto& item : units) {
      CaliperCatalogUnit unit;
      unit.name = item.at("name").get<std::string>();
      unit.symbol = item.at("symbol").get<std::string>();
      unit.dimension = item.at("dimension").get<std::string>();
      const auto unit_id = item.at("unit_id").get<std::string>();
      auto persisted = units_by_id.find(unit_id);
      const auto dimension_id = item.at("dimension_id").get<std::string>();
      auto dimension = dimensions_by_id.find(dimension_id);
      if (persisted == units_by_id.end() || dimension == dimensions_by_id.end()
          || !catalog_dimension_ids.contains(dimension_id) || dimension->second != unit.dimension) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper base catalog reference");
      }
      try {
        const auto persisted_unit = nlohmann::json::from_cbor(persisted->second.payload_cbor);
        if (persisted->second.type != *unit_type
            || persisted_unit.at("symbol").get<std::string>() != unit.symbol
            || persisted_unit.at("name").get<std::string>() != unit.name
            || persisted_unit.at("dimension_id").get<std::string>() != dimension_id) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err("Caliper base catalog reference mismatch");
        }
        const auto systems = item.contains("systems") ? item.at("systems")
            : (item.contains("system") ? nlohmann::json::array({item.at("system")})
                                        : nlohmann::json::array());
        const auto stored_systems = persisted_unit.contains("systems") ? persisted_unit.at("systems")
            : (persisted_unit.contains("system") ? nlohmann::json::array({persisted_unit.at("system")})
                                                  : nlohmann::json::array());
        if (systems != stored_systems
            || item.value("scale", 1.0) != persisted_unit.value("scale", 1.0)
            || item.value("offset", 0.0) != persisted_unit.value("offset", 0.0)
            || item.value("base_unit_id", std::string{})
                 != persisted_unit.value("base_unit_id", std::string{})) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "Caliper base catalog snapshot mismatch");
        }
      } catch (const std::exception&) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper base Unit record");
      }
      unit.object_id = persisted->second.ref.id;
      unit.base_unit = true;
      if (const auto minimum = minimum_by_dimension_id.find(dimension_id);
          minimum != minimum_by_dimension_id.end()) {
        unit.minimum_root_value = minimum->second;
      }
      if (const auto exact = exact_transforms_by_unit_id.find(unit_id);
          exact != exact_transforms_by_unit_id.end()) {
        unit.exact_scale = exact->second.first;
        unit.exact_offset = exact->second.second;
      }
      if (unit.name.empty() || unit.symbol.empty() || !dimension_names.contains(unit.dimension)) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper unit in base catalog");
      }
      if (item.contains("systems")) {
        for (const auto& system : item.at("systems")) unit.systems.push_back(system.get<std::string>());
      } else if (item.contains("system")) {
        unit.systems.push_back(item.at("system").get<std::string>());
      }
      if (unit.systems.empty()) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("unit has no Caliper system tag: " + unit.symbol);
      }
      if (item.contains("base_unit_id")) {
        const auto base_id = item.at("base_unit_id").get<std::string>();
        if (!base_unit_ids.contains(base_id)) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "Caliper base catalog Unit references a non-catalog base Unit");
        }
        auto base = units_by_id.find(base_id);
        if (base == units_by_id.end()) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper base-unit reference");
        }
        unit.base_object_id = base->second.ref.id;
        const auto base_payload = nlohmann::json::from_cbor(base->second.payload_cbor);
        const auto base_dimension_id = base_payload.at("dimension_id").get<std::string>();
        auto base_dimension = dimensions_by_id.find(base_dimension_id);
        if (base_dimension == dimensions_by_id.end() || base_dimension->second != unit.dimension) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "dimension mismatch in Caliper base-unit reference: " + unit.symbol);
        }
        unit.base_symbol = base_payload.at("symbol").get<std::string>();
      }
      if (item.contains("scale")) unit.scale = item.at("scale").get<double>();
      if (item.contains("offset")) unit.offset = item.at("offset").get<double>();
      if (!names.insert(unit.name).second || !by_symbol.emplace(unit.symbol, std::move(unit)).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("duplicate Caliper unit name or symbol");
      }
    }
    static const std::set<std::string> prefixable_symbols = {
      "m", "kg", "g", "s", "A", "K", "mol", "cd", "rad", "sr", "Hz", "N", "Pa", "J",
      "W", "C", "V", "F", "Ω", "S", "Wb", "T", "H", "°C", "lm", "lx", "Bq", "Gy",
      "Sv", "kat"
    };
    std::vector<CaliperCatalogUnit> generated;
    for (const auto& [symbol, exponent] : prefix_exponents) {
      const auto prefix_name = expected_prefix_names.at(symbol);
      for (const auto& base_symbol : prefixable_symbols) {
        if (base_symbol == "kg") continue;
        const auto composition_base = base_symbol == "g" ? std::string("g") : base_symbol;
        const auto base = by_symbol.find(composition_base);
        if (base == by_symbol.end()) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "missing SI prefix composition base: " + composition_base);
        }
        const auto composed_symbol = symbol + composition_base;
        const auto composed_name = prefix_name + base->second.name;
        const auto composed_scale = std::pow(10.0, exponent);
        auto same_composition = [&](const CaliperCatalogUnit& existing) {
          return existing.name == composed_name
              && existing.dimension == base->second.dimension
              && existing.systems == base->second.systems
              && existing.base_symbol == std::optional<std::string>(composition_base)
              && existing.scale == std::optional<double>(composed_scale)
              && existing.offset == std::optional<double>(0.0);
        };
        auto symbol_collision = by_symbol.find(composed_symbol);
        if (symbol_collision != by_symbol.end()) {
          const auto& existing = symbol_collision->second;
          const bool kilogram_base = composed_symbol == "kg"
              && existing.name == "kilogram"
              && existing.dimension == base->second.dimension
              && existing.systems == base->second.systems
              && !existing.base_symbol.has_value()
              && existing.scale.value_or(1.0) == 1.0
              && existing.offset.value_or(0.0) == 0.0;
          if (!kilogram_base && !same_composition(existing)) {
            return referee::Result<std::vector<CaliperCatalogUnit>>::err(
                "Caliper unit conflicts with SI prefix composition: " + composed_symbol);
          }
          continue;
        }
        if (names.contains(composed_name)) {
          auto name_collision = std::find_if(by_symbol.begin(), by_symbol.end(), [&](const auto& entry) {
            return entry.second.name == composed_name;
          });
          if (name_collision == by_symbol.end() || !same_composition(name_collision->second)) {
            return referee::Result<std::vector<CaliperCatalogUnit>>::err(
                "Caliper unit name conflicts with SI prefix composition: " + composed_name);
          }
          continue;
        }
        CaliperCatalogUnit unit;
        unit.name = composed_name;
        unit.symbol = composed_symbol;
        unit.dimension = base->second.dimension;
        unit.systems = base->second.systems;
        unit.base_symbol = composition_base;
        unit.scale = composed_scale;
        unit.offset = 0.0;
        unit.minimum_root_value = base->second.minimum_root_value;
        generated.push_back(std::move(unit));
      }
    }
    for (auto& unit : generated) {
      names.insert(unit.name);
      by_symbol.emplace(unit.symbol, std::move(unit));
    }
    std::vector<CaliperCatalogUnit> result;
    result.reserve(by_symbol.size());
    for (auto& [symbol, unit] : by_symbol) {
      (void)symbol;
      result.push_back(std::move(unit));
    }

    std::vector<CaliperCatalogUnit> extensions;
    for (const auto& record : *unit_records.value) {
      if (base_unit_ids.contains(record.ref.id.to_hex())) continue;
      const auto item = nlohmann::json::from_cbor(record.payload_cbor);
      // These two symbols were used by retired legacy aliases in stores created
      // before the canonical Coulomb and Farad records. Bootstrap already
      // excludes those exact historical rows when reseeding.
      const auto legacy_symbol = item.value("symbol", std::string{});
      const auto legacy_name = item.value("name", std::string{});
      const auto schema_version = unit_schema_versions.at(record.ref.id.to_hex());
      if (schema_version == 1
          && ((legacy_symbol == "C" && legacy_name == "celsius")
              || (legacy_symbol == "F" && legacy_name == "fahrenheit"))) continue;
      CaliperCatalogUnit unit;
      unit.object_id = record.ref.id;
      unit.name = item.at("name").get<std::string>();
      unit.symbol = item.at("symbol").get<std::string>();
      const auto dimension_id = item.at("dimension_id").get<std::string>();
      auto dimension = dimensions_by_id.find(dimension_id);
      if (dimension == dimensions_by_id.end()) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "unit references an unknown Caliper dimension: " + unit.symbol);
      }
      unit.dimension = dimension->second;
      if (const auto minimum = minimum_by_dimension_id.find(dimension_id);
          minimum != minimum_by_dimension_id.end()) {
        unit.minimum_root_value = minimum->second;
      }
      if (item.contains("systems")) {
        for (const auto& system : item.at("systems")) unit.systems.push_back(system.get<std::string>());
      } else if (item.contains("system")) {
        unit.systems.push_back(item.at("system").get<std::string>());
      }
      if (item.contains("base_unit_id")) {
        const auto base_id = item.at("base_unit_id").get<std::string>();
        auto base = units_by_id.find(base_id);
        if (base == units_by_id.end()) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "unit references an unknown Caliper base unit: " + unit.symbol);
        }
        const auto base_payload = nlohmann::json::from_cbor(base->second.payload_cbor);
        const auto base_dimension_id = base_payload.at("dimension_id").get<std::string>();
        auto base_dimension = dimensions_by_id.find(base_dimension_id);
        if (base_dimension == dimensions_by_id.end() || base_dimension->second != unit.dimension) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "dimension mismatch in Caliper extension base reference: " + unit.symbol);
        }
        unit.base_object_id = base->second.ref.id;
        unit.base_symbol = base_payload.at("symbol").get<std::string>();
      }
      if (item.contains("scale")) unit.scale = item.at("scale").get<double>();
      if (item.contains("offset")) unit.offset = item.at("offset").get<double>();
      unit.override_base = item.value("override_base", false);
      extensions.push_back(std::move(unit));
    }
    std::sort(extensions.begin(), extensions.end(), [](const auto& left, const auto& right) {
      if (left.name != right.name) return left.name < right.name;
      return left.symbol < right.symbol;
    });
    auto composed = compose_caliper_catalog(result, extensions);
    if (!composed) return composed;

    std::set<std::string> effective_ids;
    for (const auto& unit : *composed.value) {
      if (unit.object_id) effective_ids.insert(unit.object_id->to_hex());
    }
    for (auto unit : result) {
      if (unit.object_id && !effective_ids.contains(unit.object_id->to_hex())) {
        unit.hidden = true;
        composed.value->push_back(std::move(unit));
      }
    }
    return composed;
  } catch (const std::exception&) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper base catalog payload");
  }
}

referee::Result<referee::ObjectRecord> write_caliper_extension_unit(
    SchemaRegistry& registry,
    referee::SqliteStore& store,
    CaliperCatalogUnit unit) {
  if (unit.object_id.has_value() || unit.hidden) {
    return referee::Result<referee::ObjectRecord>::err(
        referee::ErrorCode::InvalidArgument, "Caliper extension writes cannot select an object identity");
  }
  auto types = registry.list_types();
  if (!types) return referee::Result<referee::ObjectRecord>::err(types.error.value());
  std::optional<referee::TypeID> unit_type;
  std::optional<referee::TypeID> dimension_type;
  for (const auto& type : *types.value) {
    if (type.namespace_name != "Caliper") continue;
    if (type.name == "Unit") unit_type = type.type_id;
    if (type.name == "Dimension") dimension_type = type.type_id;
  }
  if (!unit_type || !dimension_type) {
    return referee::Result<referee::ObjectRecord>::err("Caliper unit schemas are not registered");
  }
  auto unit_definition = registry.get_latest_definition_by_type(*unit_type);
  if (!unit_definition) return referee::Result<referee::ObjectRecord>::err(unit_definition.error.value());
  if (unit_definition.value->definition.version != 2) {
    return referee::Result<referee::ObjectRecord>::err(
        referee::ErrorCode::FailedPrecondition, "Caliper Unit v2 schema is not registered");
  }

  auto loaded = load_caliper_catalog(registry, store);
  if (!loaded) return referee::Result<referee::ObjectRecord>::err(loaded.error.value());
  std::vector<CaliperCatalogUnit> visible;
  visible.reserve(loaded.value->size());
  std::map<std::string, const CaliperCatalogUnit*> by_object_id;
  for (const auto& existing : *loaded.value) {
    if (existing.object_id) by_object_id.emplace(existing.object_id->to_hex(), &existing);
    if (!existing.hidden) visible.push_back(existing);
  }

  auto existing_symbol = std::find_if(visible.begin(), visible.end(), [&](const CaliperCatalogUnit& candidate) {
    return candidate.symbol == unit.symbol;
  });
  if (unit.override_base
      && (existing_symbol == visible.end() || !existing_symbol->base_unit
          || existing_symbol->override_base)) {
    return referee::Result<referee::ObjectRecord>::err(
        referee::ErrorCode::InvalidArgument,
        "Caliper extension override must target one unmodified base unit");
  }

  if (unit.base_object_id) {
    auto target = by_object_id.find(unit.base_object_id->to_hex());
    if (target == by_object_id.end()) {
      return referee::Result<referee::ObjectRecord>::err(
          referee::ErrorCode::NotFound, "Caliper extension base object ID was not found");
    }
    if (!unit.base_symbol.has_value()) unit.base_symbol = target->second->symbol;
    if (unit.base_symbol.value() != target->second->symbol) {
      return referee::Result<referee::ObjectRecord>::err(
          referee::ErrorCode::InvalidArgument, "Caliper extension base symbol does not match its object ID");
    }
    if (unit.dimension != target->second->dimension) {
      return referee::Result<referee::ObjectRecord>::err(
          referee::ErrorCode::InvalidArgument, "Caliper extension base dimension mismatch");
    }
  } else if (unit.base_symbol) {
    return referee::Result<referee::ObjectRecord>::err(
        referee::ErrorCode::InvalidArgument,
        "Caliper extension base references must use a direct object ID");
  }

  auto validation_base = visible;
  for (auto& existing : validation_base) existing.override_base = false;
  auto composed = compose_caliper_catalog(validation_base, { unit });
  if (!composed) return referee::Result<referee::ObjectRecord>::err(composed.error.value());

  auto dimensions = store.list_by_type(*dimension_type);
  if (!dimensions) return referee::Result<referee::ObjectRecord>::err(dimensions.error.value());
  std::optional<referee::ObjectID> dimension_id;
  try {
    for (const auto& record : *dimensions.value) {
      const auto payload = nlohmann::json::from_cbor(record.payload_cbor);
      if (payload.at("name").get<std::string>() != unit.dimension) continue;
      if (dimension_id) {
        return referee::Result<referee::ObjectRecord>::err(
            referee::ErrorCode::InvalidArgument, "ambiguous Caliper extension dimension");
      }
      dimension_id = record.ref.id;
    }
  } catch (const std::exception&) {
    return referee::Result<referee::ObjectRecord>::err("invalid Caliper dimension record");
  }
  if (!dimension_id) {
    return referee::Result<referee::ObjectRecord>::err(
        referee::ErrorCode::NotFound, "Caliper extension dimension was not found");
  }

  nlohmann::json payload;
  payload["name"] = unit.name;
  payload["symbol"] = unit.symbol;
  payload["dimension_id"] = dimension_id->to_hex();
  payload["systems"] = unit.systems;
  if (unit.systems.size() == 1) payload["system"] = unit.systems.front();
  if (unit.base_object_id) payload["base_unit_id"] = unit.base_object_id->to_hex();
  if (unit.scale) payload["scale"] = *unit.scale;
  if (unit.offset) payload["offset"] = *unit.offset;
  payload["override_base"] = unit.override_base;
  return store.create_object(*unit_type, unit_definition.value->ref.id,
                             nlohmann::json::to_cbor(payload));
}

referee::Result<CaliperConversionResult> convert_caliper_value(
    const std::vector<CaliperCatalogUnit>& catalog,
    std::string_view value,
    std::string_view from_unit,
    std::string_view to_unit) {
  const auto* source = find_caliper_unit(catalog, from_unit);
  if (source == nullptr) {
    return referee::Result<CaliperConversionResult>::err("unknown Caliper unit: " + std::string(from_unit));
  }
  const auto* target = find_caliper_unit(catalog, to_unit);
  if (target == nullptr) {
    return referee::Result<CaliperConversionResult>::err("unknown Caliper unit: " + std::string(to_unit));
  }
  if (source->dimension != target->dimension) {
    return referee::Result<CaliperConversionResult>::err("incompatible Caliper dimensions: "
                                                         + source->dimension + " and " + target->dimension);
  }

  auto input = parse_decimal_rational(value);
  if (!input) return referee::Result<CaliperConversionResult>::err(input.error->message);
  auto source_transform = transform_to_root(catalog, *source);
  if (!source_transform) return referee::Result<CaliperConversionResult>::err(source_transform.error->message);
  auto target_transform = transform_to_root(catalog, *target);
  if (!target_transform) return referee::Result<CaliperConversionResult>::err(target_transform.error->message);

  bool at_minimum = false;
  if (source->minimum_root_value) {
    auto constraint_root = multiply_rational(*input.value, source_transform.value->scale);
    if (!constraint_root) return referee::Result<CaliperConversionResult>::err(constraint_root.error->message);
    constraint_root = add_rational(*constraint_root.value, source_transform.value->offset);
    if (!constraint_root) return referee::Result<CaliperConversionResult>::err(constraint_root.error->message);
    auto bound = normalize_rational(source->minimum_root_value->numerator,
                                    source->minimum_root_value->denominator);
    if (!bound) return referee::Result<CaliperConversionResult>::err(bound.error->message);
    auto relative_to_bound = subtract_rational(*constraint_root.value, *bound.value);
    if (!relative_to_bound) {
      return referee::Result<CaliperConversionResult>::err(relative_to_bound.error->message);
    }
    if (relative_to_bound.value->numerator < 0) {
      return referee::Result<CaliperConversionResult>::err(
          "Caliper value is below the dimension minimum");
    }
    at_minimum = relative_to_bound.value->numerator == 0;
  }

  referee::Result<Rational> root_value = at_minimum
      ? normalize_rational(source->minimum_root_value->numerator,
                           source->minimum_root_value->denominator)
      : multiply_rational(*input.value, source_transform.value->scale);
  if (!root_value) return referee::Result<CaliperConversionResult>::err(root_value.error->message);
  if (!at_minimum) {
    root_value = add_rational(*root_value.value, source_transform.value->offset);
    if (!root_value) return referee::Result<CaliperConversionResult>::err(root_value.error->message);
  }
  auto target_offset_removed = subtract_rational(*root_value.value, target_transform.value->offset);
  if (!target_offset_removed) {
    return referee::Result<CaliperConversionResult>::err(target_offset_removed.error->message);
  }
  auto target_value = divide_rational(*target_offset_removed.value, target_transform.value->scale);
  if (!target_value) return referee::Result<CaliperConversionResult>::err(target_value.error->message);
  auto converted = rational_to_double(*target_value.value);
  if (!converted) return referee::Result<CaliperConversionResult>::err(converted.error->message);

  return referee::Result<CaliperConversionResult>::ok(CaliperConversionResult{
      *converted.value, source->dimension, source->symbol, target->symbol});
}

} // namespace iris::refract
