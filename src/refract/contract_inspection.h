#pragma once

#include "refract/operation_registry.h"

namespace iris::refract {
referee::Result<std::string> inspect_contract_catalog(
    OperationCatalog& catalog, std::optional<std::string> category = {},
    std::optional<std::string> capability = {});
referee::Result<std::string> inspect_type_contracts(
    SchemaRegistry& schema, OperationCatalog& catalog, referee::TypeID type);
}
