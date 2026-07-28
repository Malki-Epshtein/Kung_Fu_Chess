#pragma once
#include "IMessageHandler.h"

class ClientSessionRegistry;
class IClientSessionStore;

// The WebSocket-side counterpart of EnterRoomHandler for the Play (FindGame)
// path: establishes this connection's ClientSession from a token (minted by
// the Gateway's POST /login) WITHOUT claiming a room seat. FindGameHandler
// still requires sessions_.sessionFor(hdl) - since LOGIN itself no longer
// runs on this connection under the HTTP-based login flow, the client sends
// AUTH right after connecting so that lookup succeeds.
class AuthHandler : public IMessageHandler {
public:
    AuthHandler(ClientSessionRegistry& clientSessions, IClientSessionStore& sessionStore)
        : clientSessions_(clientSessions), sessionStore_(sessionStore) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    ClientSessionRegistry& clientSessions_;
    IClientSessionStore&   sessionStore_;
};
