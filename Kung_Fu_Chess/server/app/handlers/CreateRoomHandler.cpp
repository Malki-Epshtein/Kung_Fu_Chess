#include "CreateRoomHandler.h"
#include "../session/SessionRegistry.h"
#include "../session/GameSession.h"
#include "../session/RoleName.h"
#include "../logic/StartingBoard.h"
#include "../../../shared/model/Piece.h"
#include <iostream>

nlohmann::json CreateRoomHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string name = payload.at("name").get<std::string>();
    if (registry_.roomExists(name))
        return { {"success", false}, {"message", "room already exists"} };

    registry_.createRoom(name, makeStartingBoard(), bus_, /*simultaneousMode=*/true);
    attachBroadcaster_(name);//פה נוצרת ההרשמה
    // The creator becomes the room's first occupant (White) - otherwise
    // they'd have to immediately send a separate JoinRoom right after
    // creating it.
    registry_.joinRoom(name, hdl);
    Chess::Color role = registry_.roleOf(hdl);
    std::cout << "[server] room '" << name << "' created, creator assigned role: "
               << roleName(role) << std::endl;
    // moveLog is always empty here (the room was just created) - included
    // anyway for shape consistency with JoinRoom/FindGame's replies, which
    // is exactly what a late joiner needs (see GameSession::fullMoveLog).
    return { {"success", true}, {"message", "room created"}, {"role", roleName(role)}, {"roomName", name},
             {"moveLog", registry_.room(name)->fullMoveLog()} };
}
