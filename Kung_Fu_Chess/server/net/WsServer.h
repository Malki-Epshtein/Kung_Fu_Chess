#pragma once
#include <cstdint>

class SessionRegistry;
class EventBus;
class IUserRepository;
class IClientSessionStore;

// How often every room's GameSession ticks by default - also the
// granularity of the game clock itself (see MotionPath's travel-time math).
constexpr int kDefaultTickMs = 30;

// Owns the WebSocket server itself: creates it, wires connection/message
// callbacks to the objects that actually handle them (ConnectionHandler,
// MessageDispatcher, BroadcasterManager), drives every room's tick loop,
// and starts accepting connections. Contains no protocol or game logic of
// its own - see MessageDispatcher for message routing and ConnectionHandler
// for connect/disconnect handling.
class WsServer {
public:
    void run(uint16_t port, SessionRegistry& registry, EventBus& bus, IUserRepository& users,
             IClientSessionStore* sessionStore = nullptr, int tickMs = kDefaultTickMs);
};
