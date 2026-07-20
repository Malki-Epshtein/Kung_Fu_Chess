#pragma once
#include "IMessageHandler.h"
#include <functional>
#include <string>

class SessionRegistry;
class EventBus;

class CreateRoomHandler : public IMessageHandler {
public:
    // Same "attach a NetworkBroadcaster to a newly created room" step
    // FindGameHandler also needs - passed in rather than duplicated, since
    // it closes over the server's connection-sending logic that only
    // WsServer.cpp has.
    using AttachBroadcaster = std::function<void(const std::string&)>;

    CreateRoomHandler(SessionRegistry& registry, EventBus& bus, AttachBroadcaster attachBroadcaster)
        : registry_(registry), bus_(bus), attachBroadcaster_(std::move(attachBroadcaster)) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    SessionRegistry&  registry_;
    EventBus&          bus_;
    AttachBroadcaster  attachBroadcaster_;
};
