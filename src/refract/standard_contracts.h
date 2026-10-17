#pragma once

#include "refract/contract_dispatch.h"
#include "refract/bootstrap.h"

namespace iris::refract {

enum class StandardContract { ToString, Compare, Add };
ContractKey standard_contract_key(StandardContract kind);
std::vector<OperationContract> standard_operation_contracts();
referee::Result<BootstrapResult> bootstrap_standard_contracts(
    SchemaRegistry& schema, OperationCatalog& catalog);
// Bind the compiled primitive implementations to a fresh runtime dispatcher.
referee::Result<void> bind_standard_contracts(ContractDispatcher& dispatcher);

} // namespace iris::refract
