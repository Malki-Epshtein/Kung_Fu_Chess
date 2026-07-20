#pragma once
#include "IMessageHandler.h"

class UserRepository;
class ClientSessionRegistry;

// Identity operation - never touches a room/GameEngine.
class LoginHandler : public IMessageHandler {
public:
    LoginHandler(UserRepository& users, ClientSessionRegistry& sessions)
        : users_(users), sessions_(sessions) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    UserRepository&        users_;
    ClientSessionRegistry& sessions_;
};
