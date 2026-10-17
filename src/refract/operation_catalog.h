#pragma once

#include "refract/contract_types.h"
#include "refract/schema_registry.h"

namespace iris::refract {

struct ContractParameter {
  std::string name;
  referee::TypeID type{};
  std::string type_parameter;
  bool optional{false};
};

struct OperationContract {
  ContractKey key;
  std::string name;
  std::string category;
  OperationScope scope{OperationScope::Object};
  std::vector<std::string> type_parameters;
  std::vector<ContractParameter> inputs;
  std::vector<ContractParameter> outputs;
  DocumentationMetadata documentation;
  std::vector<OperationEffect> effects;
  std::vector<std::string> required_capabilities;
};

constexpr referee::TypeID kOperationContractType{0x524652434f000001ULL};

// Serialized calls, as with SchemaRegistry. Store must outlive the catalog.
class OperationCatalog {
public:
  explicit OperationCatalog(referee::SqliteStore& store) : store_(store) {}
  referee::Result<OperationContract> register_contract(const OperationContract& contract);
  referee::Result<OperationContract> get(ContractKey key);
  referee::Result<std::vector<OperationContract>> list(
      std::optional<std::string> category = {},
      std::optional<std::string> capability = {});
private:
  referee::SqliteStore& store_;
};

} // namespace iris::refract
