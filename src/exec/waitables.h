#pragma once

#include "ceo/task_registry.h"
#include "referee/referee.h"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace iris::exec {

struct WaitResult {
  bool ready{false};
  std::vector<ceo::TaskID> woken;
};

class Waitable {
public:
  virtual ~Waitable() = default;
  virtual WaitResult wait(ceo::TaskID task) = 0;
};

class Event final : public Waitable {
public:
  explicit Event(bool initially_set = false);

  WaitResult wait(ceo::TaskID task) override;
  WaitResult signal();
  void reset();
  bool is_set() const { return set_; }

private:
  bool set_{false};
  std::deque<ceo::TaskID> waiters_;
};

class Semaphore final : public Waitable {
public:
  explicit Semaphore(std::uint64_t initial = 0);

  WaitResult wait(ceo::TaskID task) override;
  WaitResult signal(std::uint64_t count = 1);
  std::uint64_t available() const { return count_; }

private:
  std::uint64_t count_{0};
  std::deque<ceo::TaskID> waiters_;
};

class Mutex final : public Waitable {
public:
  Mutex() = default;

  WaitResult wait(ceo::TaskID task) override;
  WaitResult unlock(ceo::TaskID task);
  std::optional<ceo::TaskID> owner() const { return owner_; }

private:
  std::optional<ceo::TaskID> owner_;
  std::deque<ceo::TaskID> waiters_;
};

class Future final : public Waitable {
public:
  Future() = default;

  WaitResult wait(ceo::TaskID task) override;
  WaitResult set_value(std::string value);
  bool ready() const { return value_.has_value(); }
  std::optional<std::string> value() const { return value_; }

private:
  std::optional<std::string> value_;
  std::deque<ceo::TaskID> waiters_;
};

} // namespace iris::exec
