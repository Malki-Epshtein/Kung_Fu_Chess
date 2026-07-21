#include "RoomIdentityResolver.h"

RoomIdentity RoomIdentityResolver::resolve(const SessionRegistry& sessionRegistry,
                                            const ClientSessionRegistry& clientSessions,
                                            const std::string& roomName) {
    RoomIdentity identity;

    for (const auto& hdl : sessionRegistry.connectionsInRoom(roomName)) {
        Chess::Color role = sessionRegistry.roleOf(hdl);
        const ClientSession* session = clientSessions.sessionFor(hdl);

        if (role == Chess::Color::None) {
            ++identity.spectatorCount;
        } else if (role == Chess::Color::White && session) {
            identity.whiteUsername = session->username;
            identity.whiteElo = session->elo;
        } else if (role == Chess::Color::Black && session) {
            identity.blackUsername = session->username;
            identity.blackElo = session->elo;
        }
    }

    return identity;
}
