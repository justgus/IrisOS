#include "exec/await.h"

namespace iris::exec {

referee::Result<WaitResult> await_task(Waitable& waitable, ceo::TaskRegistry& registry, ceo::TaskID task) {
  auto taskR = registry.get_task(task);
  if (!taskR) return referee::Result<WaitResult>::err(taskR.error->message);
  if (!taskR.value->has_value()) return referee::Result<WaitResult>::err("task not found");

  const auto& rec = taskR.value->value();
  if (rec.state == ceo::TaskState::CancelRequested) {
    auto canceled = registry.mark_canceled(task);
    if (!canceled) return referee::Result<WaitResult>::err(canceled.error->message);
    return referee::Result<WaitResult>::ok(WaitResult{true, {}});
  }

  auto waitR = waitable.wait(task);
  if (!waitR.ready) {
    auto waitSet = registry.wait_task(task);
    if (!waitSet) return referee::Result<WaitResult>::err(waitSet.error->message);
  }
  return referee::Result<WaitResult>::ok(std::move(waitR));
}

AwaitOutcome handle_wait_result(ceo::TaskRegistry& registry, const WaitResult& result) {
  AwaitOutcome out;
  for (auto task_id : result.woken) {
    auto taskR = registry.get_task(task_id);
    if (!taskR || !taskR.value->has_value()) continue;

    const auto& rec = taskR.value->value();
    if (rec.state == ceo::TaskState::CancelRequested) {
      if (registry.mark_canceled(task_id)) out.canceled.push_back(task_id);
      continue;
    }
    if (registry.resume_task(task_id)) out.resumed.push_back(task_id);
  }
  return out;
}

} // namespace iris::exec
