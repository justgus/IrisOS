#include "ceo/task_registry.h"

#include <algorithm>

namespace iris::ceo {

namespace {

bool is_terminal(TaskState state) {
  return state == TaskState::Canceled || state == TaskState::Completed || state == TaskState::Failed ||
         state == TaskState::Killed;
}

bool can_transition(TaskState from, TaskState to) {
  if (is_terminal(from)) return false;
  switch (to) {
    case TaskState::Waiting: return from == TaskState::Running;
    case TaskState::Running: return from == TaskState::Waiting || from == TaskState::Created;
    case TaskState::CancelRequested:
      return from == TaskState::Created || from == TaskState::Running || from == TaskState::Waiting;
    case TaskState::Canceled: return from == TaskState::CancelRequested;
    case TaskState::Completed: return from == TaskState::Running || from == TaskState::Waiting;
    case TaskState::Failed: return from == TaskState::Running || from == TaskState::Waiting;
    case TaskState::Killed:
      return from == TaskState::Created || from == TaskState::Running || from == TaskState::Waiting ||
             from == TaskState::CancelRequested;
    case TaskState::Created: return false;
  }
  return false;
}

ChildOwnership default_ownership(TaskMode mode) {
  return mode == TaskMode::Service ? ChildOwnership::Detached : ChildOwnership::Owned;
}

} // namespace

referee::Result<TaskRecord> TaskRegistry::spawn_task(const referee::ObjectID& object_id,
                                                     std::optional<TaskID> parent,
                                                     std::string name) {
  return spawn_task(object_id, parent, std::move(name), TaskMode::Inline);
}

referee::Result<TaskRecord> TaskRegistry::spawn_task(const referee::ObjectID& object_id,
                                                     std::optional<TaskID> parent,
                                                     std::string name,
                                                     TaskMode mode) {
  return spawn_task(object_id, parent, std::move(name), mode, default_ownership(mode));
}

referee::Result<TaskRecord> TaskRegistry::spawn_task(const referee::ObjectID& object_id,
                                                     std::optional<TaskID> parent,
                                                     std::string name,
                                                     TaskMode mode,
                                                     ChildOwnership ownership) {
  if (parent.has_value() && !find_task(*parent)) {
    return referee::Result<TaskRecord>::err("parent task not found");
  }

  TaskRecord rec;
  rec.id = next_id_++;
  rec.object_id = object_id;
  rec.parent = parent;
  rec.state = TaskState::Running;
  rec.mode = mode;
  rec.name = std::move(name);

  auto insert = tasks_.emplace(rec.id, rec);
  if (!insert.second) {
    return referee::Result<TaskRecord>::err("failed to insert task");
  }

  if (parent.has_value()) {
    auto* parent_task = find_task(*parent);
    if (parent_task) {
      parent_task->children.push_back(ChildLink{rec.id, ownership});
    }
  }

  return referee::Result<TaskRecord>::ok(insert.first->second);
}

referee::Result<void> TaskRegistry::wait_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Waiting)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::Waiting;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::resume_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Running)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::Running;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::cancel_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (rec->state == TaskState::CancelRequested) return referee::Result<void>::ok();
  if (!can_transition(rec->state, TaskState::CancelRequested)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::CancelRequested;
  for (const auto& child : rec->children) {
    if (child.ownership == ChildOwnership::Owned) {
      cancel_task(child.id);
    }
  }
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::mark_canceled(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Canceled)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::Canceled;
  detach_from_parent(*rec);
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::kill_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Killed)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::Killed;
  detach_from_parent(*rec);
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::complete_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Completed)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::Completed;
  detach_from_parent(*rec);
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::fail_task(TaskID id, std::string reason) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Failed)) {
    return referee::Result<void>::err("invalid state transition");
  }
  rec->state = TaskState::Failed;
  if (!reason.empty()) rec->name = std::move(reason);
  detach_from_parent(*rec);
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

void TaskRegistry::detach_from_parent(TaskRecord& rec) {
  if (!rec.parent.has_value()) return;
  auto* parent = find_task(rec.parent.value());
  if (!parent) return;
  auto& children = parent->children;
  children.erase(std::remove_if(children.begin(), children.end(),
                                [&](const ChildLink& child) { return child.id == rec.id; }),
                 children.end());
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
