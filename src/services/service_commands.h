#pragma once

#include "parser/conch_command.h"
#include "services/service_host.h"

#include <ostream>

namespace iris::service {

class ServiceCommands {
public:
  explicit ServiceCommands(ServiceHostManager& hosts) : hosts_(hosts) {}
  void execute(const parser::ServiceCommand& command, referee::SqliteStore& store,
               referee::ObjectID sender, std::ostream& output);

private:
  ServiceHostManager& hosts_;
  std::map<std::string, std::unique_ptr<MemoryService>> handlers_;
};

} // namespace iris::service
