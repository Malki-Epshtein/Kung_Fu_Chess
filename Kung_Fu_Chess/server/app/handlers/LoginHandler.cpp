#include "LoginHandler.h"
#include "../../db/UserRepository.h"
#include "../session/ClientSessionRegistry.h"
#include "../../../shared/log/Log.h"

nlohmann::json LoginHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string username = payload.at("username").get<std::string>();
    std::string password = payload.at("password").get<std::string>();
    LoginResult result = users_.login(username, password);
    spdlog::info("login {} for '{}': {}", result.success ? "OK" : "FAILED", username, result.message);
    if (result.success)
        sessions_.onLogin(hdl, username, result.elo);
    return { {"success", result.success}, {"message", result.message}, {"elo", result.elo} };
}
