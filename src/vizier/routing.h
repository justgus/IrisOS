#pragma once

#include "refract/schema_registry.h"

#include <optional>
#include <string>

namespace iris::vizier {

struct Route {
  std::string concho;
};

std::optional<Route> route_for_type(const iris::refract::TypeSummary& summary);

} // namespace iris::vizier
