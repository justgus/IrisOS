#include "exec/waitables.h"

#include <algorithm>

namespace iris::exec {

Event::Event(bool initially_set) : set_(initially_set) {}

WaitResult Event::wait(ceo::TaskID task) {
  if (set_) return WaitResult{true, {}};
  waiters_.push_back(task);
  return WaitResult{false, {}};
}

WaitResult Event::signal() {
  set_ = true;
  WaitResult out;
  out.ready = true;
  out.woken.assign(waiters_.begin(), waiters_.end());
  waiters_.clear();
  return out;
}

void Event::reset() {
  set_ = false;
}

Semaphore::Semaphore(std::uint64_t initial) : count_(initial) {}

WaitResult Semaphore::wait(ceo::TaskID task) {
  if (count_ > 0) {
    --count_;
    return WaitResult{true, {}};
  }
  waiters_.push_back(task);
  return WaitResult{false, {}};
}

WaitResult Semaphore::signal(std::uint64_t count) {
  WaitResult out;
  if (count == 0) return out;

  std::uint64_t remaining = count;
  while (remaining > 0 && !waiters_.empty()) {
    out.woken.push_back(waiters_.front());
    waiters_.pop_front();
    --remaining;
  }
  count_ += remaining;
  out.ready = true;
  return out;
}

WaitResult Mutex::wait(ceo::TaskID task) {
  if (!owner_.has_value()) {
    owner_ = task;
    return WaitResult{true, {}};
  }
  if (owner_.value() == task) return WaitResult{true, {}};
  waiters_.push_back(task);
  return WaitResult{false, {}};
}

WaitResult Mutex::unlock(ceo::TaskID task) {
  if (!owner_.has_value() || owner_.value() != task) {
    return WaitResult{false, {}};
  }
  if (waiters_.empty()) {
    owner_.reset();
    return WaitResult{true, {}};
  }
  auto next = waiters_.front();
  waiters_.pop_front();
  owner_ = next;
  return WaitResult{true, {next}};
}

WaitResult Future::wait(ceo::TaskID task) {
  if (value_.has_value()) return WaitResult{true, {}};
  waiters_.push_back(task);
  return WaitResult{false, {}};
}

WaitResult Future::set_value(std::string value) {
  if (value_.has_value()) return WaitResult{false, {}};
  value_ = std::move(value);
  WaitResult out;
  out.ready = true;
  out.woken.assign(waiters_.begin(), waiters_.end());
  waiters_.clear();
  return out;
}

} // namespace iris::exec
