#pragma once
#include "IMessageHandler.h"

class IUserRepository;
class ClientSessionRegistry;

// Identity operation - never touches a room/GameEngine.
class LoginHandler : public IMessageHandler {
public:
    LoginHandler(IUserRepository& users, ClientSessionRegistry& sessions)
        : users_(users), sessions_(sessions) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    IUserRepository&       users_;
    ClientSessionRegistry& sessions_;
};
