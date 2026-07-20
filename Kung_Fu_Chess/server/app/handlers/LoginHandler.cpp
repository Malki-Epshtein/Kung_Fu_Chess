#include "LoginHandler.h"
#include "../../db/UserRepository.h"
#include "../session/ClientSessionRegistry.h"
#include <iostream>

nlohmann::json LoginHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string username = payload.at("username").get<std::string>();
    std::string password = payload.at("password").get<std::string>();
    LoginResult result = users_.login(username, password);
    std::cout << "[server] login " << (result.success ? "OK" : "FAILED")
               << " for '" << username << "': " << result.message << std::endl;
    if (result.success)
        sessions_.onLogin(hdl, username, result.elo);
    return { {"success", result.success}, {"message", result.message}, {"elo", result.elo} };
}
