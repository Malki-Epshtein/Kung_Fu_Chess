#include "JoinRoomHandler.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../session/RoleName.h"
#include "../session/RoomIdentityResolver.h"
#include "../../../shared/model/Piece.h"
#include <iostream>

nlohmann::json JoinRoomHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string name = payload.at("name").get<std::string>();
    if (!registry_.roomExists(name))
        return { {"success", false}, {"message", "room not found"} };

    registry_.joinRoom(name, hdl);
    Chess::Color role = registry_.roleOf(hdl);
    std::cout << "[server] joined room '" << name << "', assigned role: "
               << roleName(role) << std::endl;
    // moveLog backfills a connection joining a room already in progress
    // (e.g. a spectator) - it never received the MOVE_LOGGED pushes that
    // already happened, so this is the one-time catch-up (see
    // GameSession::fullMoveLog). Everything after this reply arrives as a
    // normal live MOVE_LOGGED push, same as for anyone else in the room.
    RoomIdentity identity = RoomIdentityResolver::resolve(registry_, clientSessions_, name);
    return { {"success", true}, {"message", "joined room"}, {"role", roleName(role)}, {"roomName", name},
             {"moveLog", registry_.room(name)->fullMoveLog()},//שלחתי את כל המהחלכים עד עכשיו למי שמצטרף עכשיו
             {"whiteName", identity.whiteUsername}, {"whiteElo", identity.whiteElo},
             {"blackName", identity.blackUsername}, {"blackElo", identity.blackElo} };
}
