#pragma once

#include "referee/referee.h"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace iris::comms {
class Channel;
class DatagramPort;
} // namespace iris::comms

namespace iris::ceo {

using TaskID = std::uint64_t;

enum class TaskState {
  Created,
  Running,
  Waiting,
  CancelRequested,
  Canceled,
  Completed,
  Failed,
  Killed
};

enum class TaskMode {
  Inline,
  Service
};

enum class ChildOwnership {
  Owned,
  Detached
};

struct ChildLink {
  TaskID id{0};
  ChildOwnership ownership{ChildOwnership::Owned};
};

struct TaskRecord {
  TaskID id{0};
  referee::ObjectID object_id{};
  std::optional<referee::ObjectID> capability_context_id;
  std::optional<TaskID> parent;
  std::vector<ChildLink> children;
  TaskState state{TaskState::Created};
  TaskMode mode{TaskMode::Inline};
  std::string name;
};

struct TaskProfile {
  TaskID id{0};
  referee::ObjectID object_id{};
  std::optional<referee::ObjectID> capability_context_id;
  std::string name;
  TaskMode mode{TaskMode::Inline};
  TaskState state{TaskState::Created};
  std::uint64_t created_at_ns{0};
  std::uint64_t running_ns{0};
  std::uint64_t waiting_ns{0};
  std::uint64_t run_count{0};
  std::uint64_t wait_count{0};
  std::uint64_t cancel_count{0};
};

struct TaskProfileSnapshot {
  std::uint64_t captured_at_ns{0};
  std::vector<TaskProfile> tasks;
};

struct TaskTraceEvent {
  std::uint64_t seq{0};
  TaskID id{0};
  TaskState from{TaskState::Created};
  TaskState to{TaskState::Created};
  std::uint64_t timestamp_ns{0};
};

struct TaskTraceSnapshot {
  std::uint64_t captured_at_ns{0};
  std::uint64_t dropped_events{0};
  std::vector<TaskTraceEvent> events;
};

class TaskRegistry {
public:
  TaskRegistry() = default;

  referee::Result<TaskRecord> spawn_task(const referee::ObjectID& object_id,
                                         std::optional<TaskID> parent = std::nullopt,
                                         std::string name = {});
  referee::Result<TaskRecord> spawn_task(const referee::ObjectID& object_id,
                                         std::optional<TaskID> parent,
                                         std::string name,
                                         TaskMode mode);
  referee::Result<TaskRecord> spawn_task(const referee::ObjectID& object_id,
                                         std::optional<TaskID> parent,
                                         std::string name,
                                         TaskMode mode,
                                         ChildOwnership ownership);

  referee::Result<TaskRecord> create_task(const referee::ObjectID& object_id,
                                          std::optional<TaskID> parent = std::nullopt,
                                          std::string name = {});
  referee::Result<TaskRecord> create_task(const referee::ObjectID& object_id,
                                          std::optional<TaskID> parent,
                                          std::string name,
                                          TaskMode mode);
  referee::Result<TaskRecord> create_task(const referee::ObjectID& object_id,
                                          std::optional<TaskID> parent,
                                          std::string name,
                                          TaskMode mode,
                                          ChildOwnership ownership);
  referee::Result<void> start_task(TaskID id);
  referee::Result<void> stop_task(TaskID id);
  referee::Result<void> wait_task(TaskID id);
  referee::Result<void> resume_task(TaskID id);
  referee::Result<void> cancel_task(TaskID id);
  referee::Result<void> mark_canceled(TaskID id);
  referee::Result<void> kill_task(TaskID id);
  referee::Result<void> complete_task(TaskID id);
  referee::Result<void> fail_task(TaskID id, std::string reason);
  referee::Result<void> attach_capability_context(TaskID id, referee::ObjectID capability_context_id);
  referee::Result<void> clear_capability_context(TaskID id);

  referee::Result<std::optional<TaskRecord>> get_task(TaskID id) const;
  referee::Result<std::vector<TaskRecord>> list_tasks() const;
  referee::Result<std::vector<TaskRecord>> list_tasks_for_capability_context(
      referee::ObjectID capability_context_id) const;
  TaskProfileSnapshot profile_snapshot() const;
  TaskTraceSnapshot trace_snapshot() const;
  void clear_trace();

private:
  struct TaskProfileState {
    std::uint64_t created_at_ns{0};
    std::uint64_t last_change_ns{0};
    std::uint64_t running_ns{0};
    std::uint64_t waiting_ns{0};
    std::uint64_t run_count{0};
    std::uint64_t wait_count{0};
    std::uint64_t cancel_count{0};
    TaskState state{TaskState::Created};
  };

  std::uint64_t now_ns() const;
  void record_state_transition(TaskID id, TaskState from, TaskState to);
  void record_trace_event(TaskID id, TaskState from, TaskState to, std::uint64_t timestamp_ns);
  TaskRecord* find_task(TaskID id);
  const TaskRecord* find_task(TaskID id) const;
  void detach_from_parent(TaskRecord& rec);
  referee::Result<TaskRecord> insert_task(const referee::ObjectID& object_id,
                                          std::optional<TaskID> parent,
                                          std::string name,
                                          TaskMode mode,
                                          ChildOwnership ownership,
                                          TaskState initial_state);

private:
  TaskID next_id_{1};
  std::unordered_map<TaskID, TaskRecord> tasks_;
  std::unordered_map<TaskID, TaskProfileState> profiles_;
  std::vector<TaskTraceEvent> trace_events_;
  std::uint64_t trace_seq_{1};
  std::uint64_t trace_dropped_{0};
  std::size_t trace_capacity_{2048};
  std::chrono::steady_clock::time_point profiler_start_{std::chrono::steady_clock::now()};
};

const char* to_string(TaskState state);

class TaskComms {
public:
  explicit TaskComms(TaskRegistry& registry);

  referee::Result<std::pair<comms::Channel, comms::Channel>> open_channel(TaskID a, TaskID b);
  referee::Result<std::pair<comms::DatagramPort, comms::DatagramPort>> open_datagram(TaskID a, TaskID b);

  referee::Result<void> close_channel(comms::Channel& channel);
  referee::Result<void> close_datagram(comms::DatagramPort& port);

private:
  TaskRegistry& registry_;
};

} // namespace iris::ceo
