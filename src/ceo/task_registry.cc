#include "ceo/task_registry.h"

#include "comms/primitives.h"

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
  return insert_task(object_id, parent, std::move(name), mode, ownership, TaskState::Running);
}

referee::Result<TaskRecord> TaskRegistry::create_task(const referee::ObjectID& object_id,
                                                      std::optional<TaskID> parent,
                                                      std::string name) {
  return create_task(object_id, parent, std::move(name), TaskMode::Inline);
}

referee::Result<TaskRecord> TaskRegistry::create_task(const referee::ObjectID& object_id,
                                                      std::optional<TaskID> parent,
                                                      std::string name,
                                                      TaskMode mode) {
  return create_task(object_id, parent, std::move(name), mode, default_ownership(mode));
}

referee::Result<TaskRecord> TaskRegistry::create_task(const referee::ObjectID& object_id,
                                                      std::optional<TaskID> parent,
                                                      std::string name,
                                                      TaskMode mode,
                                                      ChildOwnership ownership) {
  return insert_task(object_id, parent, std::move(name), mode, ownership, TaskState::Created);
}

referee::Result<void> TaskRegistry::wait_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (!can_transition(rec->state, TaskState::Waiting)) {
    return referee::Result<void>::err("invalid state transition");
  }
  record_state_transition(rec->id, rec->state, TaskState::Waiting);
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
  record_state_transition(rec->id, rec->state, TaskState::Running);
  rec->state = TaskState::Running;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::start_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (rec->state != TaskState::Created) {
    return referee::Result<void>::err("task not in Created state");
  }
  return resume_task(id);
}

referee::Result<void> TaskRegistry::stop_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (rec->state == TaskState::Created) {
    auto cancelR = cancel_task(id);
    if (!cancelR) return cancelR;
    return mark_canceled(id);
  }
  return cancel_task(id);
}

referee::Result<void> TaskRegistry::cancel_task(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  if (rec->state == TaskState::CancelRequested) return referee::Result<void>::ok();
  if (!can_transition(rec->state, TaskState::CancelRequested)) {
    return referee::Result<void>::err("invalid state transition");
  }
  record_state_transition(rec->id, rec->state, TaskState::CancelRequested);
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
  record_state_transition(rec->id, rec->state, TaskState::Canceled);
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
  record_state_transition(rec->id, rec->state, TaskState::Killed);
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
  record_state_transition(rec->id, rec->state, TaskState::Completed);
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
  record_state_transition(rec->id, rec->state, TaskState::Failed);
  rec->state = TaskState::Failed;
  if (!reason.empty()) rec->name = std::move(reason);
  detach_from_parent(*rec);
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::attach_capability_context(
    TaskID id,
    referee::ObjectID capability_context_id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->capability_context_id = capability_context_id;
  return referee::Result<void>::ok();
}

referee::Result<void> TaskRegistry::clear_capability_context(TaskID id) {
  auto* rec = find_task(id);
  if (!rec) return referee::Result<void>::err("task not found");
  if (is_terminal(rec->state)) return referee::Result<void>::err("task already terminal");
  rec->capability_context_id.reset();
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

referee::Result<std::vector<TaskRecord>> TaskRegistry::list_tasks_for_capability_context(
    referee::ObjectID capability_context_id) const {
  std::vector<TaskRecord> out;
  out.reserve(tasks_.size());
  for (const auto& kv : tasks_) {
    if (kv.second.capability_context_id.has_value()
        && kv.second.capability_context_id.value() == capability_context_id) {
      out.push_back(kv.second);
    }
  }
  std::sort(out.begin(), out.end(), [](const TaskRecord& a, const TaskRecord& b) {
    return a.id < b.id;
  });
  return referee::Result<std::vector<TaskRecord>>::ok(std::move(out));
}

TaskProfileSnapshot TaskRegistry::profile_snapshot() const {
  TaskProfileSnapshot snapshot;
  snapshot.captured_at_ns = now_ns();
  std::vector<TaskRecord> records;
  records.reserve(tasks_.size());
  for (const auto& kv : tasks_) records.push_back(kv.second);
  std::sort(records.begin(), records.end(), [](const TaskRecord& a, const TaskRecord& b) {
    return a.id < b.id;
  });
  snapshot.tasks.reserve(records.size());
  for (const auto& rec : records) {
    TaskProfile profile;
    profile.id = rec.id;
    profile.object_id = rec.object_id;
    profile.capability_context_id = rec.capability_context_id;
    profile.name = rec.name;
    profile.mode = rec.mode;
    profile.state = rec.state;
    auto it = profiles_.find(rec.id);
    if (it != profiles_.end()) {
      const auto& state = it->second;
      profile.created_at_ns = state.created_at_ns;
      profile.run_count = state.run_count;
      profile.wait_count = state.wait_count;
      profile.cancel_count = state.cancel_count;
      profile.running_ns = state.running_ns;
      profile.waiting_ns = state.waiting_ns;
      if (rec.state == TaskState::Running && snapshot.captured_at_ns >= state.last_change_ns) {
        profile.running_ns += snapshot.captured_at_ns - state.last_change_ns;
      }
      if (rec.state == TaskState::Waiting && snapshot.captured_at_ns >= state.last_change_ns) {
        profile.waiting_ns += snapshot.captured_at_ns - state.last_change_ns;
      }
    }
    snapshot.tasks.push_back(std::move(profile));
  }
  return snapshot;
}

TaskTraceSnapshot TaskRegistry::trace_snapshot() const {
  TaskTraceSnapshot snapshot;
  snapshot.captured_at_ns = now_ns();
  snapshot.dropped_events = trace_dropped_;
  snapshot.events = trace_events_;
  return snapshot;
}

void TaskRegistry::clear_trace() {
  trace_events_.clear();
  trace_dropped_ = 0;
  trace_seq_ = 1;
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

referee::Result<TaskRecord> TaskRegistry::insert_task(const referee::ObjectID& object_id,
                                                      std::optional<TaskID> parent,
                                                      std::string name,
                                                      TaskMode mode,
                                                      ChildOwnership ownership,
                                                      TaskState initial_state) {
  if (parent.has_value() && !find_task(*parent)) {
    return referee::Result<TaskRecord>::err("parent task not found");
  }

  TaskRecord rec;
  rec.id = next_id_++;
  rec.object_id = object_id;
  rec.parent = parent;
  rec.state = initial_state;
  rec.mode = mode;
  rec.name = std::move(name);

  auto insert = tasks_.emplace(rec.id, rec);
  if (!insert.second) {
    return referee::Result<TaskRecord>::err("failed to insert task");
  }

  const auto now = now_ns();
  TaskProfileState profile_state;
  profile_state.created_at_ns = now;
  profile_state.last_change_ns = now;
  profile_state.state = initial_state;
  if (initial_state == TaskState::Running) profile_state.run_count = 1;
  if (initial_state == TaskState::Waiting) profile_state.wait_count = 1;
  profiles_.emplace(rec.id, profile_state);
  record_trace_event(rec.id, TaskState::Created, initial_state, now);

  if (parent.has_value()) {
    auto* parent_task = find_task(*parent);
    if (parent_task) {
      parent_task->children.push_back(ChildLink{rec.id, ownership});
    }
  }

  return referee::Result<TaskRecord>::ok(insert.first->second);
}

std::uint64_t TaskRegistry::now_ns() const {
  const auto delta = std::chrono::steady_clock::now() - profiler_start_;
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count());
}

void TaskRegistry::record_state_transition(TaskID id, TaskState from, TaskState to) {
  auto it = profiles_.find(id);
  if (it == profiles_.end()) return;
  auto now = now_ns();
  auto& state = it->second;
  if (from == TaskState::Running && now >= state.last_change_ns) {
    state.running_ns += now - state.last_change_ns;
  }
  if (from == TaskState::Waiting && now >= state.last_change_ns) {
    state.waiting_ns += now - state.last_change_ns;
  }
  if (to == TaskState::Running) state.run_count += 1;
  if (to == TaskState::Waiting) state.wait_count += 1;
  if (to == TaskState::CancelRequested) state.cancel_count += 1;
  state.state = to;
  state.last_change_ns = now;
  record_trace_event(id, from, to, now);
}

void TaskRegistry::record_trace_event(TaskID id, TaskState from, TaskState to, std::uint64_t timestamp_ns) {
  if (trace_events_.size() >= trace_capacity_) {
    trace_dropped_ += 1;
    return;
  }
  TaskTraceEvent event;
  event.seq = trace_seq_++;
  event.id = id;
  event.from = from;
  event.to = to;
  event.timestamp_ns = timestamp_ns;
  trace_events_.push_back(event);
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

TaskComms::TaskComms(TaskRegistry& registry) : registry_(registry) {}

referee::Result<std::pair<comms::Channel, comms::Channel>> TaskComms::open_channel(TaskID a, TaskID b) {
  auto aR = registry_.get_task(a);
  if (!aR) return referee::Result<std::pair<comms::Channel, comms::Channel>>::err(aR.error->message);
  if (!aR.value->has_value()) {
    return referee::Result<std::pair<comms::Channel, comms::Channel>>::err("task not found");
  }
  auto bR = registry_.get_task(b);
  if (!bR) return referee::Result<std::pair<comms::Channel, comms::Channel>>::err(bR.error->message);
  if (!bR.value->has_value()) {
    return referee::Result<std::pair<comms::Channel, comms::Channel>>::err("task not found");
  }

  return referee::Result<std::pair<comms::Channel, comms::Channel>>::ok(comms::Channel::loopback());
}

referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>> TaskComms::open_datagram(TaskID a, TaskID b) {
  auto aR = registry_.get_task(a);
  if (!aR) {
    return referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>>::err(
        aR.error->message);
  }
  if (!aR.value->has_value()) {
    return referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>>::err("task not found");
  }
  auto bR = registry_.get_task(b);
  if (!bR) {
    return referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>>::err(
        bR.error->message);
  }
  if (!bR.value->has_value()) {
    return referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>>::err("task not found");
  }

  return referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>>::ok(
      comms::DatagramPort::loopback());
}

referee::Result<void> TaskComms::close_channel(comms::Channel& channel) {
  channel.close();
  return referee::Result<void>::ok();
}

referee::Result<void> TaskComms::close_datagram(comms::DatagramPort& port) {
  port.close();
  return referee::Result<void>::ok();
}

} // namespace iris::ceo
