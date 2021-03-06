#include <cassert>
#include <limits>
#include <string>

#include "common/logger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace networking {

Networking::Networking(std::shared_ptr<messages::Queue> queue)
    : _queue(queue), _dist(0, std::numeric_limits<uint32_t>::max()) {
  _queue->run();

  // _subscriber.subscribe(
  //     messages::Type::kConnectionClosed,
  //     [this](const messages::Header &header, const messages::Body &body) {
  //       this->remove_connection(header, body);
  //     });
}

void Networking::remove_connection(const messages::Header &header,
                                   const messages::Body &body) {
  // from header get the peer and then the transport layer id and connection id
  // auto peer = header.peer();
  // if (!peer.has_transport_layer_id() || !peer.has_connection_id()) {
  //   LOG_WARNING << this << " Traying to remove connection but not necessary
  //   info in message::Header"; return;
  // }

  // _transport_layers[peer.transport_layer_id()]->terminated(peer.connection_id());
}

void Networking::send(std::shared_ptr<messages::Message> message,
                      ProtocolType type) {
  message->mutable_header()->set_id(_dist(_rd));
  for (auto &transport_layer : _transport_layers) {
    transport_layer->send(message, type);
  }
}

void Networking::send_unicast(std::shared_ptr<messages::Message> message,
                              ProtocolType type) {
  assert(message->header().has_peer());
  _transport_layers[message->header().peer().transport_layer_id()]
      ->send_unicast(message, type);
}

TransportLayer::ID Networking::push(
    std::shared_ptr<TransportLayer> transport_layer) {
  _transport_layers.push_back(transport_layer);
  transport_layer->run();
  const auto id = _transport_layers.size() - 1;
  transport_layer->id(id);
  return id;
}

void Networking::stop() {
  for (const auto transport_layer : _transport_layers) {
    transport_layer->stop();
  }
}

void Networking::join() {
  for (const auto transport_layer : _transport_layers) {
    transport_layer->join();
  }
}

Networking::~Networking() { _queue->quit(); }

}  // namespace networking
}  // namespace neuro
