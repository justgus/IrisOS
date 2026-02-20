#include "vizier/routing.h"

namespace iris::vizier {

static std::string display_name(const iris::refract::TypeSummary& summary) {
  if (summary.namespace_name.empty()) return summary.name;
  return summary.namespace_name + "::" + summary.name;
}

std::optional<Route> route_for_type(const iris::refract::TypeSummary& summary) {
  auto full = display_name(summary);
  if (full == "Viz::TextLog") return Route{"Log"};
  if (full == "Viz::Metric") return Route{"Metric"};
  if (full == "Viz::Table") return Route{"Table"};
  if (full == "Viz::Tree") return Route{"Tree"};
  return std::nullopt;
}

} // namespace iris::vizier
