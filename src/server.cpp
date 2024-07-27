// ————— No reconciliation? —————
// ⠀⣞⢽⢪⢣⢣⢣⢫⡺⡵⣝⡮⣗⢷⢽⢽⢽⣮⡷⡽⣜⣜⢮⢺⣜⢷⢽⢝⡽⣝
// ⠸⡸⠜⠕⠕⠁⢁⢇⢏⢽⢺⣪⡳⡝⣎⣏⢯⢞⡿⣟⣷⣳⢯⡷⣽⢽⢯⣳⣫⠇
// ⠀⠀⢀⢀⢄⢬⢪⡪⡎⣆⡈⠚⠜⠕⠇⠗⠝⢕⢯⢫⣞⣯⣿⣻⡽⣏⢗⣗⠏⠀
// ⠀⠪⡪⡪⣪⢪⢺⢸⢢⢓⢆⢤⢀⠀⠀⠀⠀⠈⢊⢞⡾⣿⡯⣏⢮⠷⠁⠀⠀
// ⠀⠀⠀⠈⠊⠆⡃⠕⢕⢇⢇⢇⢇⢇⢏⢎⢎⢆⢄⠀⢑⣽⣿⢝⠲⠉⠀⠀⠀⠀
// ⠀⠀⠀⠀⠀⡿⠂⠠⠀⡇⢇⠕⢈⣀⠀⠁⠡⠣⡣⡫⣂⣿⠯⢪⠰⠂⠀⠀⠀⠀
// ⠀⠀⠀⠀⡦⡙⡂⢀⢤⢣⠣⡈⣾⡃⠠⠄⠀⡄⢱⣌⣶⢏⢊⠂⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⠀⢝⡲⣜⡮⡏⢎⢌⢂⠙⠢⠐⢀⢘⢵⣽⣿⡿⠁⠁⠀⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⠀⠨⣺⡺⡕⡕⡱⡑⡆⡕⡅⡕⡜⡼⢽⡻⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⠀⣼⣳⣫⣾⣵⣗⡵⡱⡡⢣⢑⢕⢜⢕⡝⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⣴⣿⣾⣿⣿⣿⡿⡽⡑⢌⠪⡢⡣⣣⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⡟⡾⣿⢿⢿⢵⣽⣾⣼⣘⢸⢸⣞⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⠀⠁⠇⠡⠩⡫⢿⣝⡻⡮⣒⢽⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
// —————————————————————————————

#include <enet/enet.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstring>
#include <swarm/network/context.hpp>
#include <thread>
#include <vector>

using namespace swarm;

void network_broadcast(network::Context* context, const void* data, size_t data_size,
                       network::Context::PacketType type, bool reliable);

int main() {
  network::Context context(ENET_HOST_ANY);
  std::vector<ENetPeer*> peers;

  while (true) {
    ENetEvent event;
    while (enet_host_service(context.host, &event, 0) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          peers.push_back(event.peer);

          spdlog::info("A new client connected from %x:%u.", event.peer->address.host,
                       event.peer->address.port);

          size_t client_id = context.host->connectedPeers;

          std::vector<std::byte> buffer{
              static_cast<std::byte>(network::Context::PacketType::PACKET_PLAYER_JOIN),
              static_cast<std::byte>(client_id)};

          network_broadcast(&context, buffer.data(), buffer.size(),
                            network::Context::PACKET_PLAYER_JOIN, true);
          break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
          spdlog::info("Client {} disconnected", event.peer->incomingPeerID);
          break;
        }
        case ENET_EVENT_TYPE_RECEIVE: {
          spdlog::info("Received packet from client {}", event.peer->incomingPeerID);
          break;
        }
        case ENET_EVENT_TYPE_NONE: {
          spdlog::info("No event");
          break;
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}

void network_broadcast(network::Context* context, const void* data, size_t data_size,
                       network::Context::PacketType type, bool reliable) {
  ENetPacket* packet = enet_packet_create(NULL, data_size + sizeof(network::Context::PacketType),
                                          reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

  *(network::Context::PacketType*)packet->data = type;
  memcpy(packet->data + sizeof(network::Context::PacketType), data, data_size);

  enet_host_broadcast(context->host, 0, packet);
}
