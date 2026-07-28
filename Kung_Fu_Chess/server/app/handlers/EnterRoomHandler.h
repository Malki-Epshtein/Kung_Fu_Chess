#pragma once
#include "IMessageHandler.h"

class SessionRegistry;
class ClientSessionRegistry;
class IClientSessionStore;

// The WebSocket-side half of the HTTP-based login/room flow (API Gateway):
// resolves a `token` (minted by the Gateway's POST /login, looked up here
// instead of assuming LOGIN already ran on this exact connection - it
// didn't), establishes this connection's ClientSession from that, then
// claims a seat in `roomName` exactly like JoinRoomHandler does. Works for
// both a REST-created room (this becomes the first, White, occupant) and
// an existing one (Black, or Spectator) - the room's occupant count is
// what already decides that, same as today.
class EnterRoomHandler : public IMessageHandler {
public:
    EnterRoomHandler(SessionRegistry& registry, ClientSessionRegistry& clientSessions, IClientSessionStore& sessionStore)
        : registry_(registry), clientSessions_(clientSessions), sessionStore_(sessionStore) {}

    nlohmann::json handle(ConnectionHandle hdl, const nlohmann::json& payload) override;

private:
    SessionRegistry&        registry_;
    ClientSessionRegistry&  clientSessions_;
    IClientSessionStore&    sessionStore_;
};
