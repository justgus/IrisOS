#pragma once

#include "ceo/task_registry.h"
#include "exec/waitables.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace iris::comms {

using Bytes = std::vector<std::uint8_t>;

class ByteStream {
public:
  ByteStream() = default;

  exec::WaitResult wait_readable(ceo::TaskID task);
  std::size_t available() const { return buffer_.size(); }
  Bytes recv(std::size_t max_bytes);

  exec::WaitResult push(const Bytes& data);

private:
  exec::Event data_ready_{false};
  std::deque<std::uint8_t> buffer_{};
};

class Channel {
public:
  Channel(std::shared_ptr<ByteStream> incoming,
          std::shared_ptr<ByteStream> outgoing);

  exec::WaitResult wait_readable(ceo::TaskID task);
  std::size_t available() const;
  Bytes recv(std::size_t max_bytes);
  exec::WaitResult send(const Bytes& data);

  static std::pair<Channel, Channel> loopback();

private:
  std::shared_ptr<ByteStream> incoming_;
  std::shared_ptr<ByteStream> outgoing_;
};

class DatagramPort {
public:
  DatagramPort() = default;

  exec::WaitResult wait_readable(ceo::TaskID task);
  std::optional<Bytes> recv();
  exec::WaitResult send(const Bytes& data);

  static std::pair<DatagramPort, DatagramPort> loopback();

private:
  struct Mailbox {
    exec::Event data_ready{false};
    std::deque<Bytes> queue;
  };

  explicit DatagramPort(std::shared_ptr<Mailbox> inbox,
                        std::shared_ptr<Mailbox> outbox);

  std::shared_ptr<Mailbox> inbox_;
  std::shared_ptr<Mailbox> outbox_;
};

} // namespace iris::comms
