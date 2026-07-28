#include "EnterRoomHandler.h"
#include "../session/SessionRegistry.h"
#include "../session/ClientSessionRegistry.h"
#include "../session/GameSession.h"
#include "../session/RoleName.h"
#include "../session/RoomIdentityResolver.h"
#include "../../../shared/db/IClientSessionStore.h"
#include "../../../shared/model/Piece.h"
#include "../../../shared/log/Log.h"

nlohmann::json EnterRoomHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string token    = payload.at("token").get<std::string>();
    std::string roomName = payload.at("roomName").get<std::string>();

    std::optional<ClientSessionData> session = sessionStore_.get(token);
    if (!session)
        return { {"success", false}, {"message", "invalid or expired token"} };

    // Establishes identity on THIS connection/shard - LOGIN never ran here,
    // the HTTP /login call that minted `token` ran on whichever shard the
    // Gateway happened to relay it to, possibly a different one.
    clientSessions_.onLogin(hdl, session->username, session->elo);

    if (!registry_.roomExists(roomName))
        return { {"success", false}, {"message", "room not found"} };

    registry_.joinRoom(roomName, hdl);
    Chess::Color role = registry_.roleOf(hdl);
    spdlog::info("'{}' entered room '{}', assigned role: {}", session->username, roomName, roleName(role));

    RoomIdentity identity = RoomIdentityResolver::resolve(registry_, clientSessions_, roomName);
    return { {"success", true}, {"message", "entered room"}, {"role", roleName(role)}, {"roomName", roomName},
             {"moveLog", registry_.room(roomName)->fullMoveLog()},
             {"whiteName", identity.whiteUsername}, {"whiteElo", identity.whiteElo},
             {"blackName", identity.blackUsername}, {"blackElo", identity.blackElo} };
}
