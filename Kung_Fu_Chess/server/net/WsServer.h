#pragma once
#include <cstdint>

class SessionRegistry;
class EventBus;
class UserRepository;

// WebSocket server: logs connects/disconnects, decodes every incoming
// message and routes it - LOGIN goes straight to the UserRepository (an
// identity operation, not a game action); everything else goes through
// CommandDispatcher against the sender's room (via the registry) - then
// replies with the outcome. Also drives every room's tick loop on a
// periodic timer and broadcasts each resulting snapshot (published on the
// bus) to every connected client.
class WsServer {
public:
    // Stage E2: exactly one room, auto-created by the caller under this
    // name - real multi-room creation/joining arrives in Stage G.
    static constexpr const char* kDefaultRoomName = "default";

    void run(uint16_t port, SessionRegistry& registry, EventBus& bus, UserRepository& users, int tickMs = 30);
};
