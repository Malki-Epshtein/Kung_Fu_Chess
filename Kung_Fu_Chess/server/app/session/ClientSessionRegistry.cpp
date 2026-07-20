#include "ClientSessionRegistry.h"

void ClientSessionRegistry::onLogin(ConnectionHandle hdl, std::string username, int elo) {
    sessions_[hdl] = ClientSession{ std::move(username), elo };
}

void ClientSessionRegistry::onDisconnect(ConnectionHandle hdl) {
    sessions_.erase(hdl);
}

const ClientSession* ClientSessionRegistry::sessionFor(ConnectionHandle hdl) const {
    auto it = sessions_.find(hdl);
    return it == sessions_.end() ? nullptr : &it->second;
}
