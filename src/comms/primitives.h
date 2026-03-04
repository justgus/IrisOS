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

class ByteStream : public exec::Waitable {
public:
  ByteStream() = default;

  exec::WaitResult wait(ceo::TaskID task) override;
  exec::WaitResult wait_readable(ceo::TaskID task);
  std::size_t available() const { return buffer_.size(); }
  Bytes recv(std::size_t max_bytes);

  exec::WaitResult push(const Bytes& data);
  void close();
  bool closed() const { return closed_; }

private:
  bool closed_{false};
  exec::Event data_ready_{false};
  std::deque<std::uint8_t> buffer_{};
};

class Channel : public exec::Waitable {
public:
  Channel(std::shared_ptr<ByteStream> incoming,
          std::shared_ptr<ByteStream> outgoing);

  exec::WaitResult wait(ceo::TaskID task) override;
  exec::WaitResult wait_readable(ceo::TaskID task);
  std::size_t available() const;
  Bytes recv(std::size_t max_bytes);
  exec::WaitResult send(const Bytes& data);
  void close();
  bool closed() const {
    if (closed_) return true;
    if (incoming_ && incoming_->closed()) return true;
    if (outgoing_ && outgoing_->closed()) return true;
    return false;
  }

  static std::pair<Channel, Channel> loopback();

private:
  bool closed_{false};
  std::shared_ptr<ByteStream> incoming_;
  std::shared_ptr<ByteStream> outgoing_;
};

class DatagramPort : public exec::Waitable {
public:
  DatagramPort() = default;

  exec::WaitResult wait(ceo::TaskID task) override;
  exec::WaitResult wait_readable(ceo::TaskID task);
  std::optional<Bytes> recv();
  exec::WaitResult send(const Bytes& data);
  void close();
  bool closed() const;

  static std::pair<DatagramPort, DatagramPort> loopback();

private:
  struct Mailbox {
    bool closed{false};
    exec::Event data_ready{false};
    std::deque<Bytes> queue;
  };

  explicit DatagramPort(std::shared_ptr<Mailbox> inbox,
                        std::shared_ptr<Mailbox> outbox);

  std::shared_ptr<Mailbox> inbox_;
  std::shared_ptr<Mailbox> outbox_;
};

} // namespace iris::comms
