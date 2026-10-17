#pragma once

#include "refract/operation_catalog.h"

namespace iris::refract {

struct ConformingOperation {
  OperationDefinition operation;
  ContractConformance conformance;
  referee::TypeID owner{};
  std::size_t depth{0};
};

referee::Result<void> validate_contract_operations(
    OperationCatalog& catalog, const TypeDefinition& definition,
    referee::ErrorCode error_code = referee::ErrorCode::InvalidArgument);

} // namespace iris::refract
