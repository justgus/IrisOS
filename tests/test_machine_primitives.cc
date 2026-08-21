extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "machine/buffers.h"
#include "machine/scalars.h"
#include "refract/bootstrap.h"
#include "refract/schema_registry.h"
#include "referee_sqlite/sqlite_store.h"

#include <array>
#include <cstdint>
#include <optional>
#include <limits>
#include <string>
#include <vector>

using namespace iris::machine;

namespace {

template <typename T>
const char* result_message(const referee::Result<T>& result) {
  return result.error.has_value() ? result.error->message.c_str() : "ok";
}

std::optional<iris::refract::TypeSummary> find_type(
    const std::vector<iris::refract::TypeSummary>& types,
    const std::string& name) {
  for (const auto& type : types) {
    if (type.namespace_name == "Machine" && type.name == name) return type;
  }
  return std::nullopt;
}

} // namespace

START_TEST(test_validated_small_values)
{
  auto zero = Bit::create(0);
  auto one = Bit::create(1);
  auto invalid_bit = Bit::create(2);
  ck_assert_msg(zero, "zero bit rejected: %s", result_message(zero));
  ck_assert_msg(one, "one bit rejected: %s", result_message(one));
  ck_assert_uint_eq(zero.value->value(), 0U);
  ck_assert_uint_eq(one.value->value(), 1U);
  ck_assert_msg(!invalid_bit, "invalid bit accepted");
  ck_assert_int_eq(static_cast<int>(invalid_bit.error->code),
                   static_cast<int>(referee::ErrorCode::InvalidArgument));

  auto nibble = Nibble::create(0x0F);
  auto invalid_nibble = Nibble::create(0x10);
  ck_assert_msg(nibble, "nibble rejected: %s", result_message(nibble));
  ck_assert_uint_eq(nibble.value->value(), 0x0FU);
  ck_assert_msg(!invalid_nibble, "invalid nibble accepted");
}
END_TEST

START_TEST(test_words_and_uint128_are_values)
{
  ck_assert_msg(Byte(0xA5) == Byte(0xA5), "equal bytes differ");
  ck_assert_msg(Word16(0x1234) == Word16(0x1234), "equal Word16 values differ");
  ck_assert_msg(Word32(0x12345678) != Word32(0), "different Word32 values compare equal");
  ck_assert_msg(Word64(0x123456789ABCDEF0ULL) == Word64(0x123456789ABCDEF0ULL),
                "equal Word64 values differ");

  UInt128 value(0x0102030405060708ULL);
  ck_assert_uint_eq(value.bytes()[7], 0U);
  ck_assert_uint_eq(value.bytes()[8], 0x01U);
  ck_assert_uint_eq(value.bytes()[15], 0x08U);
  ck_assert_msg(value.fits(64), "64-bit value should fit in 64 bits");
  ck_assert_msg(!value.fits(56), "64-bit value should not fit in 56 bits");
}
END_TEST

START_TEST(test_alignment_validation)
{
  auto one = Alignment::create(1);
  auto page = Alignment::create(4096);
  auto zero = Alignment::create(0);
  auto non_power = Alignment::create(24);
  ck_assert_msg(one, "unit alignment rejected: %s", result_message(one));
  ck_assert_msg(page, "page alignment rejected: %s", result_message(page));
  ck_assert_uint_eq(page.value->bytes(), 4096U);
  ck_assert_msg(!zero, "zero alignment accepted");
  ck_assert_msg(!non_power, "non-power-of-two alignment accepted");
}
END_TEST

START_TEST(test_address_width_validation)
{
  auto address32 = Address::create(UInt128(0xFFFFFFFFULL), 32);
  auto overflow32 = Address::create(UInt128(0x100000000ULL), 32);
  auto width_zero = Address::create(UInt128(0), 0);
  auto width_128 = Address::create(UInt128(UInt128::Bytes{
      0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}), 128);

  ck_assert_msg(address32, "32-bit address rejected: %s", result_message(address32));
  ck_assert_uint_eq(address32.value->bit_width(), 32U);
  ck_assert_msg(!overflow32, "overflowing 32-bit address accepted");
  ck_assert_msg(!width_zero, "zero-width address accepted");
  ck_assert_msg(width_128, "128-bit address rejected: %s", result_message(width_128));
}
END_TEST

START_TEST(test_machine_scalar_refract_registration)
{
  referee::SqliteStore store(referee::SqliteConfig{.filename = ":memory:", .enable_wal = false});
  ck_assert_msg(store.open(), "store open failed");
  ck_assert_msg(store.ensure_schema(), "store schema failed");

  iris::refract::SchemaRegistry registry(store);
  auto bootstrap = iris::refract::bootstrap_core_schema(registry);
  ck_assert_msg(bootstrap, "bootstrap failed: %s", result_message(bootstrap));

  auto types = registry.list_types();
  ck_assert_msg(types, "type listing failed: %s", result_message(types));
  const std::array<const char*, 10> expected = {
      "Bit", "Nibble", "Byte", "Word16", "Word32", "Word64",
      "UInt128", "ByteOrder", "Alignment", "Address"};
  for (const auto* name : expected) {
    ck_assert_msg(find_type(*types.value, name).has_value(), "Machine::%s missing", name);
  }

  auto address = registry.get_definition_by_type(kAddressType);
  ck_assert_msg(address, "Address lookup failed: %s", result_message(address));
  ck_assert_msg(address.value->has_value(), "Address definition missing");
  ck_assert_uint_eq(address.value->value().definition.fields.size(), 2U);
  ck_assert_msg(address.value->value().definition.fields[0].type == kUInt128Type,
                "Address value does not reference Machine::UInt128");

  auto byte_order = registry.get_definition_by_type(kByteOrderType);
  ck_assert_msg(byte_order, "ByteOrder lookup failed: %s", result_message(byte_order));
  ck_assert_msg(byte_order.value->has_value(), "ByteOrder definition missing");
  ck_assert_uint_eq(byte_order.value->value().definition.enum_values.size(), 3U);

  ck_assert_msg(store.close(), "store close failed");
}
END_TEST

START_TEST(test_blob_slice_and_packet_values)
{
  Blob blob({Byte(0x10), Byte(0x20), Byte(0x30), Byte(0x40)});
  auto range = Span::create(1, 2);
  ck_assert_msg(range, "valid span rejected: %s", result_message(range));
  auto slice = Slice::create(blob, *range.value);
  ck_assert_msg(slice, "valid slice rejected: %s", result_message(slice));
  ck_assert_uint_eq(slice.value->size(), 2U);
  ck_assert_uint_eq(slice.value->bytes()[0].value(), 0x20U);
  ck_assert_uint_eq(slice.value->bytes()[1].value(), 0x30U);

  Slice retained = *slice.value;
  blob = Blob();
  ck_assert_uint_eq(retained.bytes()[0].value(), 0x20U);

  Packet first(Blob({Byte(0xAA), Byte(0xBB)}));
  Packet second(Blob({Byte(0xAA), Byte(0xBB)}));
  ck_assert_msg(first == second, "equal packets differ");
  ck_assert_uint_eq(first.size(), 2U);
}
END_TEST

START_TEST(test_span_and_slice_bounds)
{
  auto overflow = Span::create(std::numeric_limits<std::size_t>::max(), 1);
  ck_assert_msg(!overflow, "overflowing span accepted");

  Blob blob({Byte(1), Byte(2)});
  auto at_end = Span::create(2, 0);
  auto past_end = Span::create(2, 1);
  ck_assert_msg(at_end, "empty end span rejected: %s", result_message(at_end));
  ck_assert_msg(past_end, "representable span rejected: %s", result_message(past_end));
  auto empty = Slice::create(blob, *at_end.value);
  auto invalid = Slice::create(blob, *past_end.value);
  ck_assert_msg(empty, "empty end slice rejected: %s", result_message(empty));
  ck_assert_msg(empty.value->empty(), "empty slice reports nonempty");
  ck_assert_msg(!invalid, "out-of-bounds slice accepted");
}
END_TEST

START_TEST(test_machine_buffer_refract_registration)
{
  referee::SqliteStore store(referee::SqliteConfig{.filename = ":memory:", .enable_wal = false});
  ck_assert_msg(store.open(), "store open failed");
  ck_assert_msg(store.ensure_schema(), "store schema failed");
  iris::refract::SchemaRegistry registry(store);
  auto bootstrap = iris::refract::bootstrap_core_schema(registry);
  ck_assert_msg(bootstrap, "bootstrap failed: %s", result_message(bootstrap));

  auto types = registry.list_types();
  ck_assert_msg(types, "type listing failed: %s", result_message(types));
  for (const auto* name : {"Blob", "Span", "Slice", "Packet"}) {
    ck_assert_msg(find_type(*types.value, name).has_value(), "Machine::%s missing", name);
  }

  auto packet = registry.get_definition_by_type(kPacketType);
  ck_assert_msg(packet, "Packet lookup failed: %s", result_message(packet));
  ck_assert_msg(packet.value->has_value(), "Packet definition missing");
  ck_assert_uint_eq(packet.value->value().definition.fields.size(), 1U);
  ck_assert_msg(packet.value->value().definition.fields[0].type == kBlobType,
                "Packet payload does not reference Machine::Blob");
  ck_assert_msg(store.close(), "store close failed");
}
END_TEST

Suite* machine_primitives_suite(void) {
  Suite* suite = suite_create("MachinePrimitives");
  TCase* tests = tcase_create("core");
  tcase_add_test(tests, test_validated_small_values);
  tcase_add_test(tests, test_words_and_uint128_are_values);
  tcase_add_test(tests, test_alignment_validation);
  tcase_add_test(tests, test_address_width_validation);
  tcase_add_test(tests, test_machine_scalar_refract_registration);
  tcase_add_test(tests, test_blob_slice_and_packet_values);
  tcase_add_test(tests, test_span_and_slice_bounds);
  tcase_add_test(tests, test_machine_buffer_refract_registration);
  suite_add_tcase(suite, tests);
  return suite;
}

int main(void) {
  Suite* suite = machine_primitives_suite();
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  const int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? 0 : 1;
}
