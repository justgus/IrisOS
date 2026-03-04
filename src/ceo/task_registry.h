#pragma once

#include "referee/referee.h"
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
  std::optional<TaskID> parent;
  std::vector<ChildLink> children;
  TaskState state{TaskState::Created};
  TaskMode mode{TaskMode::Inline};
  std::string name;
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

  referee::Result<std::optional<TaskRecord>> get_task(TaskID id) const;
  referee::Result<std::vector<TaskRecord>> list_tasks() const;

private:
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
