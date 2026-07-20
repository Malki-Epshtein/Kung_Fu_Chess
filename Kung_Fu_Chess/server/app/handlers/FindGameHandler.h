#pragma once
#include "IMessageHandler.h"
#include <asio/io_context.hpp>
#include <functional>
#include <string>

class ClientSessionRegistry;
class Matchmaker;
class SessionRegistry;
class EventBus;

// The most complex handler: on a miss it also arms a 60s search-timeout
// asio::steady_timer (same pattern as SessionRegistry's disconnect
// countdown). `push` is how it reaches a connection OTHER than the one
// that called handle() - the matched opponent's GameFound push, and the
// timeout message - since a handler's return value only ever reaches the
// connection that sent the request.
class FindGameHandler : public IMessageHandler {
public:
    using AttachBroadcaster = std::function<void(const std::string&)>;
    using PushToConnection  = std::function<void(ConnectionHandle, const std::string&)>;

    FindGameHandler(ClientSessionRegistry& sessions, Matchmaker& matchmaker,
                     SessionRegistry& registry, EventBus& bus,
                     AttachBroadcaster attachBroadcaster, PushToConnection push,
                     asio::io_context& ioContext)
        : sessions_(sessions), matchmaker_(matchmaker), registry_(registry), bus_(bus),
          attachBroadcaster_(std::move(attachBroadcaster)), push_(std::move(push)),
          ioContext_(ioContext) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    ClientSessionRegistry& sessions_;
    Matchmaker&             matchmaker_;
    SessionRegistry&        registry_;
    EventBus&                bus_;
    AttachBroadcaster        attachBroadcaster_;
    PushToConnection         push_;
    asio::io_context&        ioContext_;

    // Prefixes matchmaking-created rooms' names so they can never collide
    // with a user-typed Create/Join room name - owned here now instead of
    // as a loose WsServer::run local, since only this handler ever uses it.
    int nextMatchId_ = 1;
};
