#pragma once

#include "ceo/task_registry.h"
#include "comms/primitives.h"
#include "exec/await.h"

#include <optional>
#include <unordered_map>
#include <variant>

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

namespace iris::refract {
struct DispatchMatch;
} // namespace iris::refract

namespace iris::conduit {

enum class IoHandleKind {
  Channel,
  Datagram,
  ByteStream
};

struct IoHandle {
  IoHandleKind kind{IoHandleKind::Channel};
  std::uint64_t id{0};
};

struct IoHandlePair {
  IoHandle first{};
  IoHandle second{};
};

struct IoSendResult {
  bool ready{false};
  exec::AwaitOutcome outcome{};
};

struct IoAwaitResult {
  bool ready{false};
  exec::AwaitOutcome outcome{};
};

class IoHandleStore {
public:
  IoHandleStore() = default;

  IoHandle store(comms::Channel channel);
  IoHandle store(comms::DatagramPort port);
  IoHandle store(comms::ByteStream stream);

  comms::Channel* find_channel(const IoHandle& handle);
  comms::DatagramPort* find_datagram(const IoHandle& handle);
  comms::ByteStream* find_stream(const IoHandle& handle);

  bool erase(const IoHandle& handle);

private:
  struct Entry {
    IoHandleKind kind{};
    std::variant<comms::DatagramPort, comms::ByteStream, comms::Channel> value{};
  };

  std::uint64_t next_id_{1};
  std::unordered_map<std::uint64_t, Entry> entries_{};
};

class IoExecutor {
public:
  IoExecutor(ceo::TaskRegistry& registry,
             ceo::TaskComms& comms,
             ceo::IoReactor& reactor,
             IoHandleStore& handles);

  referee::Result<IoHandlePair> open_channel(const refract::DispatchMatch& match,
                                             ceo::TaskID a,
                                             ceo::TaskID b);
  referee::Result<IoHandlePair> open_datagram(const refract::DispatchMatch& match,
                                              ceo::TaskID a,
                                              ceo::TaskID b);

  referee::Result<IoSendResult> send_channel(const refract::DispatchMatch& match,
                                             const IoHandle& handle,
                                             const comms::Bytes& data);
  referee::Result<IoSendResult> send_datagram(const refract::DispatchMatch& match,
                                              const IoHandle& handle,
                                              const comms::Bytes& data);

  referee::Result<IoAwaitResult> await_channel(const refract::DispatchMatch& match,
                                               const IoHandle& handle,
                                               ceo::TaskID task);
  referee::Result<IoAwaitResult> await_datagram(const refract::DispatchMatch& match,
                                                const IoHandle& handle,
                                                ceo::TaskID task);

  referee::Result<comms::Bytes> recv_channel(const refract::DispatchMatch& match,
                                             const IoHandle& handle,
                                             std::size_t max_bytes);
  referee::Result<std::optional<comms::Bytes>> recv_datagram(const refract::DispatchMatch& match,
                                                             const IoHandle& handle);

  referee::Result<void> close_channel(const refract::DispatchMatch& match,
                                      const IoHandle& handle);
  referee::Result<void> close_datagram(const refract::DispatchMatch& match,
                                       const IoHandle& handle);

private:
  ceo::TaskRegistry& registry_;
  ceo::TaskComms& comms_;
  ceo::IoReactor& reactor_;
  IoHandleStore& handles_;
};

} // namespace iris::conduit
