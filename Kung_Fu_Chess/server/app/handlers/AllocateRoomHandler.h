#pragma once
#include "json.hpp"
#include <asio/io_context.hpp>
#include <functional>
#include <string>

class SessionRegistry;
class EventBus;

// Answers the Game Allocator's "create/host this room here" NATS request
// (Server_Design.md step 6's Game Allocator responsibility) - the
// out-of-process equivalent of directly calling registry_.createRoom +
// attachBroadcaster. Serves two callers: a PLAY match (allocator_main.cpp's
// matchmaking.matched handler), which also reserves both seats by username
// up front so joinRoom (called later, from EnterRoomHandler, once the
// actual client reconnects here) assigns White/Black by identity instead of
// arrival order - see SessionRegistry::reserveSeats - and a regular room
// (the same handler's allocation.request, from the API Gateway's POST
// /rooms), which sends no usernames and so reserves nothing; its creator
// gets White by plain arrival order instead.
//
// Not an IMessageHandler: it never runs in response to a client's own
// WebSocket message, only a NATS request from the Allocator - registered
// directly via INatsClient::subscribeRequest (see MessageDispatcher).
// handle() runs on the cnats delivery thread but marshals the actual
// SessionRegistry work onto the WsServer's io_context and blocks until it
// finishes, since SessionRegistry isn't thread-safe (same reasoning as
// FindGameHandler's NATS subscriptions) and NATS request/reply is itself
// synchronous from the Allocator's point of view.
class AllocateRoomHandler {
public:
    using AttachBroadcaster = std::function<void(const std::string&)>;

    AllocateRoomHandler(SessionRegistry& registry, EventBus& bus, AttachBroadcaster attachBroadcaster,
                         asio::io_context& ioContext);

    nlohmann::json handle(const nlohmann::json& request);

private:
    nlohmann::json doAllocate(const nlohmann::json& request);

    SessionRegistry&  registry_;
    EventBus&          bus_;
    AttachBroadcaster  attachBroadcaster_;
    asio::io_context&  ioContext_;
};
