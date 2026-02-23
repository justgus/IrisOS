#include "comms/primitives.h"

#include <algorithm>

namespace iris::comms {

exec::WaitResult ByteStream::wait(ceo::TaskID task) {
  return wait_readable(task);
}

exec::WaitResult ByteStream::wait_readable(ceo::TaskID task) {
  if (!buffer_.empty()) return exec::WaitResult{true, {}};
  return data_ready_.wait(task);
}

Bytes ByteStream::recv(std::size_t max_bytes) {
  Bytes out;
  if (max_bytes == 0 || buffer_.empty()) return out;

  std::size_t count = std::min(max_bytes, buffer_.size());
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back(buffer_.front());
    buffer_.pop_front();
  }

  if (buffer_.empty()) data_ready_.reset();
  return out;
}

exec::WaitResult ByteStream::push(const Bytes& data) {
  if (data.empty()) return exec::WaitResult{true, {}};
  for (auto byte : data) buffer_.push_back(byte);
  return data_ready_.signal();
}

Channel::Channel(std::shared_ptr<ByteStream> incoming,
                 std::shared_ptr<ByteStream> outgoing)
  : incoming_(std::move(incoming)),
    outgoing_(std::move(outgoing)) {}

exec::WaitResult Channel::wait(ceo::TaskID task) {
  return wait_readable(task);
}

exec::WaitResult Channel::wait_readable(ceo::TaskID task) {
  return incoming_->wait_readable(task);
}

std::size_t Channel::available() const {
  return incoming_->available();
}

Bytes Channel::recv(std::size_t max_bytes) {
  return incoming_->recv(max_bytes);
}

exec::WaitResult Channel::send(const Bytes& data) {
  return outgoing_->push(data);
}

std::pair<Channel, Channel> Channel::loopback() {
  auto a_to_b = std::make_shared<ByteStream>();
  auto b_to_a = std::make_shared<ByteStream>();

  Channel a(b_to_a, a_to_b);
  Channel b(a_to_b, b_to_a);
  return {a, b};
}

DatagramPort::DatagramPort(std::shared_ptr<Mailbox> inbox,
                           std::shared_ptr<Mailbox> outbox)
  : inbox_(std::move(inbox)),
    outbox_(std::move(outbox)) {}

exec::WaitResult DatagramPort::wait(ceo::TaskID task) {
  return wait_readable(task);
}

exec::WaitResult DatagramPort::wait_readable(ceo::TaskID task) {
  if (!inbox_ || !outbox_) return exec::WaitResult{false, {}};
  if (!inbox_->queue.empty()) return exec::WaitResult{true, {}};
  return inbox_->data_ready.wait(task);
}

std::optional<Bytes> DatagramPort::recv() {
  if (!inbox_ || inbox_->queue.empty()) return std::nullopt;
  auto packet = std::move(inbox_->queue.front());
  inbox_->queue.pop_front();
  if (inbox_->queue.empty()) inbox_->data_ready.reset();
  return packet;
}

exec::WaitResult DatagramPort::send(const Bytes& data) {
  if (!inbox_ || !outbox_) return exec::WaitResult{false, {}};
  outbox_->queue.push_back(data);
  return outbox_->data_ready.signal();
}

std::pair<DatagramPort, DatagramPort> DatagramPort::loopback() {
  auto inbox_a = std::make_shared<Mailbox>();
  auto inbox_b = std::make_shared<Mailbox>();
  DatagramPort a(inbox_a, inbox_b);
  DatagramPort b(inbox_b, inbox_a);
  return {a, b};
}

} // namespace iris::comms
