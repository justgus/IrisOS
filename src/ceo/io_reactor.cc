#include "ceo/io_reactor.h"

#include "refract/dispatch.h"

#include <string>

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

namespace iris::conduit {

namespace {

constexpr referee::TypeID kTypeU64{0x1002ULL};
constexpr referee::TypeID kTypeBool{0x1003ULL};
constexpr referee::TypeID kTypeBytes{0x1007ULL};

constexpr referee::TypeID kTypeKernelIo{0x4B494F5000000001ULL};
constexpr referee::TypeID kTypeKernelIoChannel{0x4B494F5000000002ULL};
constexpr referee::TypeID kTypeKernelIoDatagram{0x4B494F5000000003ULL};

struct ExpectedParam {
  referee::TypeID type{};
  bool optional{false};
};

bool params_match(const std::vector<refract::ParameterDefinition>& params,
                  std::initializer_list<ExpectedParam> expected,
                  std::string* err) {
  if (params.size() != expected.size()) {
    if (err) *err = "parameter count mismatch";
    return false;
  }
  std::size_t index = 0;
  for (const auto& exp : expected) {
    const auto& param = params[index++];
    if (param.type.v != exp.type.v) {
      if (err) *err = "parameter type mismatch";
      return false;
    }
    if (param.optional != exp.optional) {
      if (err) *err = "parameter optional mismatch";
      return false;
    }
  }
  return true;
}

referee::Result<void> validate_operation(const refract::OperationDefinition& op,
                                         const char* expected_name,
                                         refract::OperationScope expected_scope,
                                         std::initializer_list<ExpectedParam> expected_params,
                                         std::initializer_list<ExpectedParam> expected_outputs) {
  if (op.name != expected_name) {
    return referee::Result<void>::err("unexpected operation name");
  }
  if (op.scope != expected_scope) {
    return referee::Result<void>::err("unexpected operation scope");
  }
  std::string err;
  if (!params_match(op.signature.params, expected_params, &err)) {
    return referee::Result<void>::err(err);
  }
  if (!params_match(op.signature.outputs, expected_outputs, &err)) {
    return referee::Result<void>::err(err);
  }
  return referee::Result<void>::ok();
}

} // namespace

IoHandle IoHandleStore::store(comms::Channel channel) {
  IoHandle handle{IoHandleKind::Channel, next_id_++};
  entries_.emplace(handle.id, Entry{handle.kind, std::move(channel)});
  return handle;
}

IoHandle IoHandleStore::store(comms::DatagramPort port) {
  IoHandle handle{IoHandleKind::Datagram, next_id_++};
  entries_.emplace(handle.id, Entry{handle.kind, std::move(port)});
  return handle;
}

IoHandle IoHandleStore::store(comms::ByteStream stream) {
  IoHandle handle{IoHandleKind::ByteStream, next_id_++};
  entries_.emplace(handle.id, Entry{handle.kind, std::move(stream)});
  return handle;
}

comms::Channel* IoHandleStore::find_channel(const IoHandle& handle) {
  auto it = entries_.find(handle.id);
  if (it == entries_.end()) return nullptr;
  if (it->second.kind != handle.kind) return nullptr;
  return std::get_if<comms::Channel>(&it->second.value);
}

comms::DatagramPort* IoHandleStore::find_datagram(const IoHandle& handle) {
  auto it = entries_.find(handle.id);
  if (it == entries_.end()) return nullptr;
  if (it->second.kind != handle.kind) return nullptr;
  return std::get_if<comms::DatagramPort>(&it->second.value);
}

comms::ByteStream* IoHandleStore::find_stream(const IoHandle& handle) {
  auto it = entries_.find(handle.id);
  if (it == entries_.end()) return nullptr;
  if (it->second.kind != handle.kind) return nullptr;
  return std::get_if<comms::ByteStream>(&it->second.value);
}

bool IoHandleStore::erase(const IoHandle& handle) {
  auto it = entries_.find(handle.id);
  if (it == entries_.end()) return false;
  if (it->second.kind != handle.kind) return false;
  entries_.erase(it);
  return true;
}

IoExecutor::IoExecutor(ceo::TaskRegistry& registry,
                       ceo::TaskComms& comms,
                       ceo::IoReactor& reactor,
                       IoHandleStore& handles)
  : registry_(registry),
    comms_(comms),
    reactor_(reactor),
    handles_(handles) {}

referee::Result<IoHandlePair> IoExecutor::open_channel(const refract::DispatchMatch& match,
                                                       ceo::TaskID a,
                                                       ceo::TaskID b) {
  auto valid = validate_operation(match.operation, "open_channel", refract::OperationScope::Class,
                                  { {kTypeU64, false}, {kTypeU64, false} },
                                  { {kTypeKernelIoChannel, false}, {kTypeKernelIoChannel, false} });
  if (!valid) return referee::Result<IoHandlePair>::err(valid.error->message);

  auto openR = comms_.open_channel(a, b);
  if (!openR) return referee::Result<IoHandlePair>::err(openR.error->message);
  IoHandlePair out;
  out.first = handles_.store(std::move(openR.value->first));
  out.second = handles_.store(std::move(openR.value->second));
  return referee::Result<IoHandlePair>::ok(out);
}

referee::Result<IoHandlePair> IoExecutor::open_datagram(const refract::DispatchMatch& match,
                                                        ceo::TaskID a,
                                                        ceo::TaskID b) {
  auto valid = validate_operation(match.operation, "open_datagram", refract::OperationScope::Class,
                                  { {kTypeU64, false}, {kTypeU64, false} },
                                  { {kTypeKernelIoDatagram, false}, {kTypeKernelIoDatagram, false} });
  if (!valid) return referee::Result<IoHandlePair>::err(valid.error->message);

  auto openR = comms_.open_datagram(a, b);
  if (!openR) return referee::Result<IoHandlePair>::err(openR.error->message);
  IoHandlePair out;
  out.first = handles_.store(std::move(openR.value->first));
  out.second = handles_.store(std::move(openR.value->second));
  return referee::Result<IoHandlePair>::ok(out);
}

referee::Result<IoSendResult> IoExecutor::send_channel(const refract::DispatchMatch& match,
                                                       const IoHandle& handle,
                                                       const comms::Bytes& data) {
  auto valid = validate_operation(match.operation, "send", refract::OperationScope::Object,
                                  { {kTypeBytes, false} },
                                  { {kTypeBool, false} });
  if (!valid) return referee::Result<IoSendResult>::err(valid.error->message);
  if (match.owner_type.v != kTypeKernelIoChannel.v) {
    return referee::Result<IoSendResult>::err("unexpected owner type for channel send");
  }

  auto* channel = handles_.find_channel(handle);
  if (!channel) return referee::Result<IoSendResult>::err("channel handle not found");
  auto wait = channel->send(data);
  IoSendResult out;
  out.ready = wait.ready;
  out.outcome = reactor_.handle_result(wait);
  return referee::Result<IoSendResult>::ok(out);
}

referee::Result<IoSendResult> IoExecutor::send_datagram(const refract::DispatchMatch& match,
                                                        const IoHandle& handle,
                                                        const comms::Bytes& data) {
  auto valid = validate_operation(match.operation, "send", refract::OperationScope::Object,
                                  { {kTypeBytes, false} },
                                  { {kTypeBool, false} });
  if (!valid) return referee::Result<IoSendResult>::err(valid.error->message);
  if (match.owner_type.v != kTypeKernelIoDatagram.v) {
    return referee::Result<IoSendResult>::err("unexpected owner type for datagram send");
  }

  auto* port = handles_.find_datagram(handle);
  if (!port) return referee::Result<IoSendResult>::err("datagram handle not found");
  auto wait = port->send(data);
  IoSendResult out;
  out.ready = wait.ready;
  out.outcome = reactor_.handle_result(wait);
  return referee::Result<IoSendResult>::ok(out);
}

referee::Result<IoAwaitResult> IoExecutor::await_channel(const refract::DispatchMatch& match,
                                                         const IoHandle& handle,
                                                         ceo::TaskID task) {
  auto valid = validate_operation(match.operation, "await_readable", refract::OperationScope::Object,
                                  { {kTypeU64, false} },
                                  { {kTypeBool, false} });
  if (!valid) return referee::Result<IoAwaitResult>::err(valid.error->message);
  if (match.owner_type.v != kTypeKernelIoChannel.v) {
    return referee::Result<IoAwaitResult>::err("unexpected owner type for channel await");
  }

  auto* channel = handles_.find_channel(handle);
  if (!channel) return referee::Result<IoAwaitResult>::err("channel handle not found");
  auto waitR = reactor_.await_readable(*channel, task);
  if (!waitR) return referee::Result<IoAwaitResult>::err(waitR.error->message);
  IoAwaitResult out;
  out.ready = waitR.value->ready;
  out.outcome = reactor_.handle_result(*waitR.value);
  return referee::Result<IoAwaitResult>::ok(out);
}

referee::Result<IoAwaitResult> IoExecutor::await_datagram(const refract::DispatchMatch& match,
                                                          const IoHandle& handle,
                                                          ceo::TaskID task) {
  auto valid = validate_operation(match.operation, "await_readable", refract::OperationScope::Object,
                                  { {kTypeU64, false} },
                                  { {kTypeBool, false} });
  if (!valid) return referee::Result<IoAwaitResult>::err(valid.error->message);
  if (match.owner_type.v != kTypeKernelIoDatagram.v) {
    return referee::Result<IoAwaitResult>::err("unexpected owner type for datagram await");
  }

  auto* port = handles_.find_datagram(handle);
  if (!port) return referee::Result<IoAwaitResult>::err("datagram handle not found");
  auto waitR = reactor_.await_readable(*port, task);
  if (!waitR) return referee::Result<IoAwaitResult>::err(waitR.error->message);
  IoAwaitResult out;
  out.ready = waitR.value->ready;
  out.outcome = reactor_.handle_result(*waitR.value);
  return referee::Result<IoAwaitResult>::ok(out);
}

referee::Result<comms::Bytes> IoExecutor::recv_channel(const refract::DispatchMatch& match,
                                                       const IoHandle& handle,
                                                       std::size_t max_bytes) {
  auto valid = validate_operation(match.operation, "recv", refract::OperationScope::Object,
                                  { {kTypeU64, false} },
                                  { {kTypeBytes, false} });
  if (!valid) return referee::Result<comms::Bytes>::err(valid.error->message);
  if (match.owner_type.v != kTypeKernelIoChannel.v) {
    return referee::Result<comms::Bytes>::err("unexpected owner type for channel recv");
  }

  auto* channel = handles_.find_channel(handle);
  if (!channel) return referee::Result<comms::Bytes>::err("channel handle not found");
  return referee::Result<comms::Bytes>::ok(channel->recv(max_bytes));
}

referee::Result<std::optional<comms::Bytes>> IoExecutor::recv_datagram(
    const refract::DispatchMatch& match,
    const IoHandle& handle) {
  auto valid = validate_operation(match.operation, "recv", refract::OperationScope::Object,
                                  {},
                                  { {kTypeBytes, true} });
  if (!valid) return referee::Result<std::optional<comms::Bytes>>::err(valid.error->message);
  if (match.owner_type.v != kTypeKernelIoDatagram.v) {
    return referee::Result<std::optional<comms::Bytes>>::err("unexpected owner type for datagram recv");
  }

  auto* port = handles_.find_datagram(handle);
  if (!port) return referee::Result<std::optional<comms::Bytes>>::err("datagram handle not found");
  return referee::Result<std::optional<comms::Bytes>>::ok(port->recv());
}

referee::Result<void> IoExecutor::close_channel(const refract::DispatchMatch& match,
                                                const IoHandle& handle) {
  auto valid = validate_operation(match.operation, "close", refract::OperationScope::Object,
                                  {},
                                  {});
  if (!valid) return valid;
  if (match.owner_type.v != kTypeKernelIoChannel.v) {
    return referee::Result<void>::err("unexpected owner type for channel close");
  }

  auto* channel = handles_.find_channel(handle);
  if (!channel) return referee::Result<void>::err("channel handle not found");
  channel->close();
  handles_.erase(handle);
  return referee::Result<void>::ok();
}

referee::Result<void> IoExecutor::close_datagram(const refract::DispatchMatch& match,
                                                 const IoHandle& handle) {
  auto valid = validate_operation(match.operation, "close", refract::OperationScope::Object,
                                  {},
                                  {});
  if (!valid) return valid;
  if (match.owner_type.v != kTypeKernelIoDatagram.v) {
    return referee::Result<void>::err("unexpected owner type for datagram close");
  }

  auto* port = handles_.find_datagram(handle);
  if (!port) return referee::Result<void>::err("datagram handle not found");
  port->close();
  handles_.erase(handle);
  return referee::Result<void>::ok();
}

} // namespace iris::conduit
