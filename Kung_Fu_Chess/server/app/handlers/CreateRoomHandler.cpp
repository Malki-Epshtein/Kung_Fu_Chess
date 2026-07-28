#include "CreateRoomHandler.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../session/RoleName.h"
#include "../session/RoomIdentityResolver.h"
#include "../logic/StartingBoard.h"
#include "../../../shared/model/Piece.h"
#include "../../../shared/log/Log.h"

nlohmann::json CreateRoomHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string name = payload.at("name").get<std::string>();
    if (registry_.roomExists(name))
        return { {"success", false}, {"message", "room already exists"} };

    registry_.createRoom(name, makeStartingBoard(), bus_, /*simultaneousMode=*/true);
    attachBroadcaster_(name);//פה נוצרת ההרשמה

    // The API Gateway's internal relay (POST /rooms) sets this false: it
    // creates the room shell over a throwaway connection that's about to
    // close, and closing it would immediately free (and likely delete) an
    // auto-assigned seat. The real client claims the seat later, over its
    // own WebSocket, via EnterRoom - see EnterRoomHandler.
    bool autoJoin = payload.value("autoJoin", true);
    if (!autoJoin)
        return { {"success", true}, {"message", "room created"}, {"roomName", name} };

    // The creator becomes the room's first occupant (White) - otherwise
    // they'd have to immediately send a separate JoinRoom right after
    // creating it.
    registry_.joinRoom(name, hdl);
    Chess::Color role = registry_.roleOf(hdl);
    spdlog::info("room '{}' created, creator assigned role: {}", name, roleName(role));
    // moveLog is always empty here (the room was just created) - included
    // anyway for shape consistency with JoinRoom/FindGame's replies, which
    // is exactly what a late joiner needs (see GameSession::fullMoveLog).
    RoomIdentity identity = RoomIdentityResolver::resolve(registry_, clientSessions_, name);
    return { {"success", true}, {"message", "room created"}, {"role", roleName(role)}, {"roomName", name},
             {"moveLog", registry_.room(name)->fullMoveLog()},
             {"whiteName", identity.whiteUsername}, {"whiteElo", identity.whiteElo},
             {"blackName", identity.blackUsername}, {"blackElo", identity.blackElo} };
}
