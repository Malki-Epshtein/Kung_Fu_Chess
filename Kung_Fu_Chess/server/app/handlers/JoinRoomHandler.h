#pragma once
#include "IMessageHandler.h"

class SessionRegistry;
class ClientSessionRegistry;

class JoinRoomHandler : public IMessageHandler {
public:
    JoinRoomHandler(SessionRegistry& registry, ClientSessionRegistry& clientSessions)
        : registry_(registry), clientSessions_(clientSessions) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    SessionRegistry&        registry_;
    ClientSessionRegistry&  clientSessions_;
};
