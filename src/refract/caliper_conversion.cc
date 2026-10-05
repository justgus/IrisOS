#include "refract/bootstrap.h"

#include <array>
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

  std::int64_t mantissa = 0;
  std::size_t fractional_digits = 0;
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
    std::int64_t scaled = 0;
    if (!checked_multiply(mantissa, 10, &scaled)
        || !checked_add(scaled, static_cast<std::int64_t>(c - '0'), &mantissa)) {
      return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
    }
    if (saw_decimal) ++fractional_digits;
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

  const auto fractional = static_cast<int>(fractional_digits);
  const int decimal_power = exponent - fractional;
  if (decimal_power > 18 || decimal_power < -18) {
    return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
  }
  std::int64_t denominator = 1;
  if (decimal_power > 0) {
    for (int i = 0; i < decimal_power; ++i) {
      if (!checked_multiply(mantissa, 10, &mantissa)) {
        return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
      }
    }
  } else {
    for (int i = 0; i > decimal_power; --i) {
      if (!checked_multiply(denominator, 10, &denominator)) {
        return referee::Result<Rational>::err("exact rational Caliper conversion overflow");
      }
    }
  }
  if (negative) mantissa = -mantissa;
  return normalize_rational(mantissa, denominator);
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
  const long double converted = static_cast<long double>(value.numerator)
                                / static_cast<long double>(value.denominator);
  const double result = static_cast<double>(converted);
  if (!std::isfinite(result)) return referee::Result<double>::err("Caliper conversion result is outside the numeric range");
  return referee::Result<double>::ok(result);
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
  while (current->base_symbol.has_value()) {
    if (!visited.insert(current->symbol).second) {
      return referee::Result<AffineTransform>::err("cycle in Caliper unit conversion chain");
    }
    auto parent = find_caliper_unit(catalog, *current->base_symbol);
    if (parent == nullptr) {
      return referee::Result<AffineTransform>::err("unknown base unit in Caliper conversion chain: "
                                                   + *current->base_symbol);
    }
    if (parent->dimension != unit.dimension) {
      return referee::Result<AffineTransform>::err("dimension mismatch in Caliper conversion chain");
    }
    auto step_scale = rational_for_factor(current->scale, 1.0);
    if (!step_scale) return referee::Result<AffineTransform>::err(step_scale.error->message);
    auto step_offset = rational_for_offset(current->offset);
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
  if (!visited.insert(current->symbol).second) {
    return referee::Result<AffineTransform>::err("cycle in Caliper unit conversion chain");
  }
  return referee::Result<AffineTransform>::ok(transform);
}

} // namespace

referee::Result<std::vector<CaliperCatalogUnit>> load_caliper_catalog(
    SchemaRegistry& registry,
    referee::SqliteStore& store) {
  auto types = registry.list_types();
  if (!types) return referee::Result<std::vector<CaliperCatalogUnit>>::err(types.error->message);
  std::optional<referee::TypeID> dimension_type;
  std::optional<referee::TypeID> unit_type;
  for (const auto& type : *types.value) {
    if (type.namespace_name != "Caliper") continue;
    if (type.name == "Dimension") dimension_type = type.type_id;
    if (type.name == "Unit") unit_type = type.type_id;
  }
  if (!dimension_type || !unit_type) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("Caliper unit schemas are not registered");
  }

  auto dimensions = store.list_by_type(*dimension_type);
  if (!dimensions) return referee::Result<std::vector<CaliperCatalogUnit>>::err(dimensions.error->message);
  std::map<std::string, std::string> dimension_name_by_id;
  try {
    for (const auto& record : *dimensions.value) {
      auto payload = nlohmann::json::from_cbor(record.payload_cbor);
      const auto name = payload.at("name").get<std::string>();
      if (name.empty() || !dimension_name_by_id.emplace(record.ref.id.to_hex(), name).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid or duplicate Caliper dimension");
      }
    }
  } catch (const std::exception&) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper dimension record");
  }

  auto records = store.list_by_type(*unit_type);
  if (!records) return referee::Result<std::vector<CaliperCatalogUnit>>::err(records.error->message);
  std::map<std::string, std::string> symbol_by_id;
  try {
    for (const auto& record : *records.value) {
      auto payload = nlohmann::json::from_cbor(record.payload_cbor);
      const auto symbol = payload.at("symbol").get<std::string>();
      const auto name = payload.value("name", std::string{});
      if ((symbol == "C" && name == "celsius") || (symbol == "F" && name == "fahrenheit")) continue;
      if (symbol.empty() || !symbol_by_id.emplace(record.ref.id.to_hex(), symbol).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper unit symbol");
      }
    }

    std::map<std::string, CaliperCatalogUnit> by_symbol;
    std::set<std::string> names;
    for (const auto& record : *records.value) {
      auto payload = nlohmann::json::from_cbor(record.payload_cbor);
      CaliperCatalogUnit unit;
      unit.name = payload.at("name").get<std::string>();
      unit.symbol = payload.at("symbol").get<std::string>();
      if ((unit.symbol == "C" && unit.name == "celsius")
          || (unit.symbol == "F" && unit.name == "fahrenheit")) continue;
      const auto dimension_id = payload.at("dimension_id").get<std::string>();
      auto dimension = dimension_name_by_id.find(dimension_id);
      if (dimension == dimension_name_by_id.end()) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err(
            "unit references an unknown Caliper dimension: " + unit.symbol);
      }
      unit.dimension = dimension->second;
      if (payload.contains("systems")) {
        for (const auto& system : payload.at("systems")) unit.systems.push_back(system.get<std::string>());
      } else if (payload.contains("system")) {
        unit.systems.push_back(payload.at("system").get<std::string>());
      }
      if (unit.systems.empty()) return referee::Result<std::vector<CaliperCatalogUnit>>::err(
          "unit has no Caliper system tag: " + unit.symbol);
      if (payload.contains("base_unit_id")) {
        const auto base_id = payload.at("base_unit_id").get<std::string>();
        auto base = symbol_by_id.find(base_id);
        if (base == symbol_by_id.end()) {
          return referee::Result<std::vector<CaliperCatalogUnit>>::err(
              "unit references an unknown Caliper base unit: " + unit.symbol);
        }
        unit.base_symbol = base->second;
      }
      if (payload.contains("scale")) unit.scale = payload.at("scale").get<double>();
      if (payload.contains("offset")) unit.offset = payload.at("offset").get<double>();
      if (unit.name.empty() || unit.symbol.empty() || unit.dimension.empty()) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper unit record");
      }
      if (!names.insert(unit.name).second || !by_symbol.emplace(unit.symbol, std::move(unit)).second) {
        return referee::Result<std::vector<CaliperCatalogUnit>>::err("duplicate Caliper unit name or symbol");
      }
    }
    std::vector<CaliperCatalogUnit> result;
    result.reserve(by_symbol.size());
    for (auto& [symbol, unit] : by_symbol) {
      (void)symbol;
      result.push_back(std::move(unit));
    }
    return referee::Result<std::vector<CaliperCatalogUnit>>::ok(std::move(result));
  } catch (const std::exception&) {
    return referee::Result<std::vector<CaliperCatalogUnit>>::err("invalid Caliper unit record");
  }
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

  auto root_value = multiply_rational(*input.value, source_transform.value->scale);
  if (!root_value) return referee::Result<CaliperConversionResult>::err(root_value.error->message);
  root_value = add_rational(*root_value.value, source_transform.value->offset);
  if (!root_value) return referee::Result<CaliperConversionResult>::err(root_value.error->message);
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
