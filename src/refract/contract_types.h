#pragma once

#include "referee/referee.h"

#include <map>
#include <string>

namespace iris::refract {

struct ContractKey {
  referee::ObjectID id{};
  std::uint64_t version{1};
  friend bool operator==(const ContractKey&, const ContractKey&) = default;
};

struct ContractConformance {
  ContractKey contract;
  std::map<std::string, referee::TypeID> substitutions;
};

enum class ExecutionRoute { Local, Service, ObjectSystem };

struct OperationBinding {
  referee::ObjectID implementation{};
  std::uint64_t version{1};
  ExecutionRoute route{ExecutionRoute::Local};
  std::string destination;
};

} // namespace iris::refract
