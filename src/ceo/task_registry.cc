#include "ceo/task_registry.h"

#include <algorithm>

namespace iris::ceo {

namespace {

bool is_terminal(TaskState state) {
  return state == TaskState::Canceled || state == TaskState::Completed || state == TaskState::Failed ||
         state == TaskState::Killed;
}

} // namespace

referee::Result<TaskRecord> TaskRegistry::spawn_task(const referee::ObjectID& object_id,
                                                     std::optional<TaskID> parent,
                                                     std::string name) {
  if (parent.has_value() && !find_task(*parent)) {
    return referee::Result<TaskRecord>::err("parent task not found");
  }

  TaskRecord rec;
  rec.id = next_id_++;
  rec.object_id = object_id;
  rec.parent = parent;
  rec.state = TaskState::Running;
  rec.name = std::move(name);

  auto insert = tasks_.emplace(rec.id, rec);
  if (!insert.second) {
    return referee::Result<TaskRecord>::err("failed to insert task");
  }

  if (parent.has_value()) {
    auto* parent_task = find_task(*parent);
    if (parent_task) parent_task->children.push_back(rec.id);
  }

  return referee::Result<TaskRecord>::ok(insert.first->second);
}

referee::Result<void> TaskRegistry::wait_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::Waiting;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::resume_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::Running;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::cancel_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::CancelRequested;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::mark_canceled(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::Canceled;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::kill_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::Killed;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::complete_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::Completed;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::fail_task(TaskID id, std::string reason) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->state = TaskState::Failed;
  if (!reason.empty()) rec->name = std::move(reason);
  return referee::Result<void>::ok();
}

referee::Result<std::optional<TaskRecord>> TaskRegistry::get_task(TaskID id) const {
  const auto* rec = find_task(id);
  if (!rec) return referee::Result<std::optional<TaskRecord>>::ok(std::nullopt);
  return referee::Result<std::optional<TaskRecord>>::ok(*rec);
}

referee::Result<std::vector<TaskRecord>> TaskRegistry::list_tasks() const {
  std::vector<TaskRecord> out;
  out.reserve(tasks_.size());
  for (const auto& kv : tasks_) out.push_back(kv.second);
  std::sort(out.begin(), out.end(), [](const TaskRecord& a, const TaskRecord& b) {
    return a.id < b.id;
  });
  return referee::Result<std::vector<TaskRecord>>::ok(std::move(out));
}

TaskRecord* TaskRegistry::find_task(TaskID id) {
  auto it = tasks_.find(id);
  return it == tasks_.end() ? nullptr : &it->second;
}

const TaskRecord* TaskRegistry::find_task(TaskID id) const {
  auto it = tasks_.find(id);
  return it == tasks_.end() ? nullptr : &it->second;
}

const char* to_string(TaskState state) {
  switch (state) {
    case TaskState::Created: return "Created";
    case TaskState::Running: return "Running";
    case TaskState::Waiting: return "Waiting";
    case TaskState::CancelRequested: return "CancelRequested";
    case TaskState::Canceled: return "Canceled";
    case TaskState::Completed: return "Completed";
    case TaskState::Failed: return "Failed";
    case TaskState::Killed: return "Killed";
  }
  return "Unknown";
}

} // namespace iris::ceo
