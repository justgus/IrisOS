#pragma once

#include "refract/operation_registry.h"
#include <functional>

namespace iris::refract {

struct ContractValue {
  referee::TypeID type{};
  referee::Bytes payload;
};
using ContractValues = std::vector<ContractValue>;
using ContractCallback = std::function<referee::Result<ContractValues>(
    const std::optional<ContractValue>& receiver, const ContractValues& inputs)>;

// Serialized invocation. Dependencies outlive this dispatcher; callbacks are process-local.
class ContractDispatcher {
public:
  ContractDispatcher(SchemaRegistry& schema, OperationCatalog& catalog)
      : schema_(schema), catalog_(catalog) {}
  referee::Result<ConformingOperation> resolve(
      referee::TypeID target, ContractKey contract, const std::vector<referee::TypeID>& inputs);
  referee::Result<void> bind(referee::ObjectID implementation, std::uint64_t version,
                             ContractCallback callback);
  referee::Result<ContractValues> invoke(
      referee::TypeID target, ContractKey contract, const ContractValues& inputs,
      const std::vector<std::string>& capabilities,
      const std::optional<ContractValue>& receiver = {});
  referee::Result<ContractValues> invoke_object(
      referee::ObjectID object, referee::SqliteStore& store, ContractKey contract,
      const ContractValues& inputs, const std::vector<std::string>& capabilities);
private:
  referee::Result<bool> subtype(referee::TypeID actual, referee::TypeID expected);
  SchemaRegistry& schema_;
  OperationCatalog& catalog_;
  std::map<std::pair<std::string, std::uint64_t>, ContractCallback> callbacks_;
};

} // namespace iris::refract
