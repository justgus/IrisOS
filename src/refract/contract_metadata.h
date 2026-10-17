#pragma once

#include "refract/schema_registry.h"
#include <nlohmann/json_fwd.hpp>

namespace iris::refract {
// Internal schema codec; validation errors remain inside the Result boundary.
void encode_contract_metadata(nlohmann::json& json, const OperationDefinition& op);
referee::Result<void> decode_contract_metadata(const nlohmann::json& json, OperationDefinition& op);
}
