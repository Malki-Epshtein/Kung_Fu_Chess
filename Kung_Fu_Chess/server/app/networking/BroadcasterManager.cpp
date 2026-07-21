#include "BroadcasterManager.h"
#include "../session/GameSession.h"
#include "../session/SessionRegistry.h"

BroadcasterManager::BroadcasterManager(EventBus& bus, SessionRegistry& registry, ConnectionSender sender)
    : bus_(bus), registry_(registry), sender_(std::move(sender)) {
    for (const std::string& roomName : registry_.roomNames())
        attach(roomName);
}

void BroadcasterManager::attach(const std::string& roomName) {
    auto forwardToRoom = [this, roomName](const std::string& text) {
        for (const auto& hdl : registry_.connectionsInRoom(roomName))
            sender_(hdl, text);
    };

    std::vector<std::unique_ptr<NetworkBroadcaster>> roomBroadcasters;
    roomBroadcasters.push_back(std::make_unique<NetworkBroadcaster>(bus_, GameSession::snapshotTopic(roomName), forwardToRoom));
    roomBroadcasters.push_back(std::make_unique<NetworkBroadcaster>(bus_, GameSession::moveLogTopic(roomName), forwardToRoom));
    broadcasters_[roomName] = std::move(roomBroadcasters);
}
