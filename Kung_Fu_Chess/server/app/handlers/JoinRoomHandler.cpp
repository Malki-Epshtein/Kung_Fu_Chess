#include "JoinRoomHandler.h"
#include "../session/SessionRegistry.h"
#include "../session/RoleName.h"
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
    return { {"success", true}, {"message", "joined room"}, {"role", roleName(role)}, {"roomName", name} };
}
