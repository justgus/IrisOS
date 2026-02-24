extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "parser/xml_parser.h"

using namespace iris::parser;

namespace {

unsigned int as_uint(std::size_t value) {
  return static_cast<unsigned int>(value);
}

} // namespace

START_TEST(test_xml_nested)
{
  const char* input = "<root id=\"r1\"><child name=\"a\">Hello &amp; goodbye</child></root>";
  auto result = parse_xml(input);
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_msg(result.root.has_value(), "expected root");

  const auto& root = *result.root;
  ck_assert_str_eq(root.name.c_str(), "root");
  ck_assert_uint_eq(as_uint(root.attributes.size()), 1U);
  ck_assert_str_eq(root.attributes[0].name.c_str(), "id");
  ck_assert_str_eq(root.attributes[0].value.c_str(), "r1");

  ck_assert_uint_eq(as_uint(root.children.size()), 1U);
  const auto& child = root.children[0];
  ck_assert_str_eq(child.name.c_str(), "child");
  ck_assert_uint_eq(as_uint(child.attributes.size()), 1U);
  ck_assert_str_eq(child.attributes[0].name.c_str(), "name");
  ck_assert_str_eq(child.attributes[0].value.c_str(), "a");

  ck_assert_uint_eq(as_uint(child.children.size()), 1U);
  const auto& text = child.children[0];
  ck_assert_int_eq(static_cast<int>(text.kind), static_cast<int>(XmlNode::Kind::Text));
  ck_assert_str_eq(text.text.c_str(), "Hello & goodbye");
}
END_TEST

START_TEST(test_xml_self_closing)
{
  const char* input = "<root><item key=\"1\"/></root>";
  auto result = parse_xml(input);
  ck_assert_uint_eq(as_uint(result.errors.size()), 0U);
  ck_assert_msg(result.root.has_value(), "expected root");
  const auto& root = *result.root;
  ck_assert_uint_eq(as_uint(root.children.size()), 1U);
  ck_assert_str_eq(root.children[0].name.c_str(), "item");
}
END_TEST

START_TEST(test_xml_error)
{
  auto result = parse_xml("<root><child></root>");
  ck_assert_msg(!result.errors.empty(), "expected parse error");
}
END_TEST

Suite* xml_parser_suite(void) {
  Suite* s = suite_create("XmlParser");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_xml_nested);
  tcase_add_test(tc, test_xml_self_closing);
  tcase_add_test(tc, test_xml_error);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = xml_parser_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
