#pragma once

#include "ceo/task_registry.h"
#include "exec/waitables.h"

namespace iris::exec {

struct AwaitOutcome {
  std::vector<ceo::TaskID> resumed;
  std::vector<ceo::TaskID> canceled;
};

referee::Result<WaitResult> await_task(Waitable& waitable, ceo::TaskRegistry& registry, ceo::TaskID task);
AwaitOutcome handle_wait_result(ceo::TaskRegistry& registry, const WaitResult& result);

} // namespace iris::exec
