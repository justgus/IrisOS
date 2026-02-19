#pragma once

#include "referee/referee.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace iris::ceo {

using TaskID = std::uint64_t;

enum class TaskState {
  Created,
  Running,
  CancelRequested,
  Canceled,
  Completed,
  Failed,
  Killed
};

struct TaskRecord {
  TaskID id{0};
  referee::ObjectID object_id{};
  std::optional<TaskID> parent;
  std::vector<TaskID> children;
  TaskState state{TaskState::Created};
  std::string name;
};

class TaskRegistry {
public:
  TaskRegistry() = default;

  referee::Result<TaskRecord> spawn_task(const referee::ObjectID& object_id,
                                         std::optional<TaskID> parent = std::nullopt,
                                         std::string name = {});
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

private:
  TaskID next_id_{1};
  std::unordered_map<TaskID, TaskRecord> tasks_;
};

const char* to_string(TaskState state);

} // namespace iris::ceo
