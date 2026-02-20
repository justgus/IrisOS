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

std::optional<Route> route_for_type_id(iris::refract::SchemaRegistry& registry,
                                       referee::TypeID type_id) {
  auto listR = registry.list_types();
  if (!listR) return std::nullopt;

  for (const auto& summary : listR.value.value()) {
    if (summary.type_id == type_id) return route_for_type(summary);
  }
  return std::nullopt;
}

} // namespace iris::vizier
