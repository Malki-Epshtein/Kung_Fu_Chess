#pragma once
#include <cstdint>

class SessionRegistry;
class EventBus;

// WebSocket server: logs connects/disconnects, decodes every incoming
// message and routes it through CommandDispatcher against the sender's room
// (via the registry), then replies with the dispatch outcome. Also drives
// every room's tick loop on a periodic timer and broadcasts each resulting
// snapshot (published on the bus) to every connected client.
class WsServer {
public:
    // Stage E2: exactly one room, auto-created by the caller under this
    // name - real multi-room creation/joining arrives in Stage G.
    static constexpr const char* kDefaultRoomName = "default";

    void run(uint16_t port, SessionRegistry& registry, EventBus& bus, int tickMs = 30);
};
