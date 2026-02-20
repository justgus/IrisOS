#pragma once

#include "refract/schema_registry.h"

#include <optional>
#include <string>

namespace iris::vizier {

struct Route {
  std::string concho;
};

std::optional<Route> route_for_type(const iris::refract::TypeSummary& summary);
std::optional<Route> route_for_type_id(iris::refract::SchemaRegistry& registry,
                                       referee::TypeID type_id);

} // namespace iris::vizier
