#pragma once
#include "IMessageHandler.h"

class SessionRegistry;

class JoinRoomHandler : public IMessageHandler {
public:
    explicit JoinRoomHandler(SessionRegistry& registry) : registry_(registry) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    SessionRegistry& registry_;
};
