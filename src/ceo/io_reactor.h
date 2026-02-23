#pragma once

#include "ceo/task_registry.h"
#include "comms/primitives.h"
#include "exec/await.h"

namespace iris::ceo {

class IoReactor {
public:
  explicit IoReactor(TaskRegistry& registry);

  referee::Result<exec::WaitResult> await_readable(comms::ByteStream& stream, TaskID task);
  referee::Result<exec::WaitResult> await_readable(comms::Channel& channel, TaskID task);
  referee::Result<exec::WaitResult> await_readable(comms::DatagramPort& port, TaskID task);

  exec::AwaitOutcome handle_result(const exec::WaitResult& result);

  exec::AwaitOutcome push(comms::ByteStream& stream, const comms::Bytes& data);
  exec::AwaitOutcome send(comms::Channel& channel, const comms::Bytes& data);
  exec::AwaitOutcome send(comms::DatagramPort& port, const comms::Bytes& data);

private:
  TaskRegistry& registry_;
};

} // namespace iris::ceo
