extern "C" {
#include <check.h>
}
#ifdef fail
#undef fail
#endif

#include "vizier/routing.h"

using iris::refract::TypeSummary;
using iris::vizier::route_for_type;

START_TEST(test_viz_routes)
{
  TypeSummary log{referee::TypeID{1}, referee::ObjectID{}, "TextLog", "Viz"};
  TypeSummary metric{referee::TypeID{2}, referee::ObjectID{}, "Metric", "Viz"};
  TypeSummary table{referee::TypeID{3}, referee::ObjectID{}, "Table", "Viz"};
  TypeSummary tree{referee::TypeID{4}, referee::ObjectID{}, "Tree", "Viz"};

  ck_assert(route_for_type(log).has_value());
  ck_assert(route_for_type(metric).has_value());
  ck_assert(route_for_type(table).has_value());
  ck_assert(route_for_type(tree).has_value());
}
END_TEST

START_TEST(test_unknown_route)
{
  TypeSummary other{referee::TypeID{5}, referee::ObjectID{}, "Panel", "Viz"};
  ck_assert(!route_for_type(other).has_value());
}
END_TEST

Suite* vizier_routing_suite(void) {
  Suite* s = suite_create("VizierRouting");
  TCase* tc = tcase_create("core");

  tcase_add_test(tc, test_viz_routes);
  tcase_add_test(tc, test_unknown_route);

  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  Suite* s = vizier_routing_suite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int failures = srunner_ntests_failed(sr);
  srunner_free(sr);
  return failures == 0 ? 0 : 1;
}
