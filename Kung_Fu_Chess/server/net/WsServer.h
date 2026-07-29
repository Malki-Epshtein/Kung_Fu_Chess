#pragma once
#include "../../shared/bus/INatsClient.h"
#include <cstdint>
#include <string>

class SessionRegistry;
class EventBus;
class IUserRepository;
class IClientSessionStore;
class IGameHistoryRepository;
class IGameStateStore;

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
    // `nats`/`shardAddress` are optional (default: matchmaking disabled -
    // FindGame degrades to "matchmaking unavailable", same pattern as a
    // null sessionStore disabling ENTER_ROOM/AUTH) - only server_main.cpp's
    // Docker/Linux build with NATS_URL set ever passes a real NatsClient.
    // `gameHistory` is likewise optional - null (native Windows, or no
    // DATABASE_URL) just means finished games aren't recorded (see
    // GameHistoryService). `gameStateStore` is the same shape again - null
    // (native Windows, or no REDIS_HOST) just means in-progress games have
    // no live Redis mirror (see GameStateMirrorService/IGameStateStore.h).
    void run(uint16_t port, SessionRegistry& registry, EventBus& bus, IUserRepository& users,
             IClientSessionStore* sessionStore = nullptr, INatsClient* nats = nullptr,
             std::string shardAddress = "", int tickMs = kDefaultTickMs,
             IGameHistoryRepository* gameHistory = nullptr, IGameStateStore* gameStateStore = nullptr);
};
