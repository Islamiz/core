#include "networking/tcp/Connection.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
namespace tcp {

std::shared_ptr<tcp::socket> Connection::socket() { return _socket; }

void Connection::read() { read_header(); }

void Connection::read_header() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [this](const boost::system::error_code &error, std::size_t bytes_read) {
        if (error) {
          this->terminate();
          return;
        }

        auto header_pattern = reinterpret_cast<HeaderPattern *>(_header.data());
        _buffer.resize(header_pattern->size);
        read_body();
      });
}

void Connection::read_body() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [this](const boost::system::error_code &error, std::size_t bytes_read) {
        if (error) {
          this->terminate();
          return;
        }

        auto header_pattern = reinterpret_cast<HeaderPattern *>(_header.data());
        auto message = std::make_shared<messages::Message>();
        messages::from_buffer(_buffer, message.get());
        auto header = message->mutable_header();
        header->mutable_peer()->CopyFrom(*_remote_peer);
        header->set_signature(&header_pattern->signature, sizeof(header_pattern->signature));

        _queue->publish(message);

        read_header();
      });
}

bool Connection::send(const Buffer &message) {
  boost::asio::async_write(*_socket,
                           boost::asio::buffer(message.data(), message.size()),
                           [this](const boost::system::error_code &error,
                                  std::size_t bytes_transferred) {
                             if (error) {
                               LOG_ERROR << "Could not send message";
                               this->terminate();
                               return false;
                             }
                             return true;
                           });
  return true;
}

void Connection::terminate() {
  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  auto peer = header->mutable_peer();
  peer->CopyFrom(*_remote_peer);
  auto body = message->add_bodies();
  body->mutable_connection_closed();
  _queue->publish(message);
}

const IP Connection::remote_ip() const {
  const auto endpoint = _socket->remote_endpoint();
  return endpoint.address();
}

const Port Connection::remote_port() const {
  const auto endpoint = _socket->remote_endpoint();
  return static_cast<Port>(endpoint.port());
}

  std::shared_ptr<const messages::Peer> Connection::remote_peer() const {
  return _remote_peer;
  // if we didn't connect to remote bot we can't know the remote port
}

} // namespace tcp
} // namespace networking
} // namespace neuro
