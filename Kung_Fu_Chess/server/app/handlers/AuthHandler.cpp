#include "AuthHandler.h"
#include "../session/ClientSessionRegistry.h"
#include "../../../shared/db/IClientSessionStore.h"
#include "../../../shared/log/Log.h"

nlohmann::json AuthHandler::handle(ConnectionHandle hdl, const nlohmann::json& payload) {
    std::string token = payload.at("token").get<std::string>();

    std::optional<ClientSessionData> session = sessionStore_.get(token);
    if (!session)
        return { {"success", false}, {"message", "invalid or expired token"} };

    clientSessions_.onLogin(hdl, session->username, session->elo);
    spdlog::info("'{}' authenticated this connection via token", session->username);
    return { {"success", true}, {"message", "authenticated"} };
}
