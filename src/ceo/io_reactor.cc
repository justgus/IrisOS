#include "ceo/io_reactor.h"

namespace iris::ceo {

IoReactor::IoReactor(TaskRegistry& registry) : registry_(registry) {}

referee::Result<exec::WaitResult> IoReactor::await_readable(comms::ByteStream& stream, TaskID task) {
  return exec::await_task(stream, registry_, task);
}

referee::Result<exec::WaitResult> IoReactor::await_readable(comms::Channel& channel, TaskID task) {
  return exec::await_task(channel, registry_, task);
}

referee::Result<exec::WaitResult> IoReactor::await_readable(comms::DatagramPort& port, TaskID task) {
  return exec::await_task(port, registry_, task);
}

exec::AwaitOutcome IoReactor::handle_result(const exec::WaitResult& result) {
  return exec::handle_wait_result(registry_, result);
}

exec::AwaitOutcome IoReactor::push(comms::ByteStream& stream, const comms::Bytes& data) {
  auto result = stream.push(data);
  return handle_result(result);
}

exec::AwaitOutcome IoReactor::send(comms::Channel& channel, const comms::Bytes& data) {
  auto result = channel.send(data);
  return handle_result(result);
}

exec::AwaitOutcome IoReactor::send(comms::DatagramPort& port, const comms::Bytes& data) {
  auto result = port.send(data);
  return handle_result(result);
}

} // namespace iris::ceo
