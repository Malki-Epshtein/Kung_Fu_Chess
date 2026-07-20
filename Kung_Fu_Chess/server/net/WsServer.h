#pragma once
#include <cstdint>

class GameSession;
class EventBus;

// WebSocket server: logs connects/disconnects, decodes every incoming
// message and routes it through CommandDispatcher against the session's
// engine, then replies with the dispatch outcome. Also drives the session's
// tick loop on a periodic timer and broadcasts each resulting snapshot
// (published on the bus) to every connected client.
class WsServer {
public:
    void run(uint16_t port, GameSession& session, EventBus& bus, int tickMs = 30);
};
