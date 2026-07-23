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

    // Snapshot delivery is direct now (state, not a one-off event - see
    // GameSession's class comment) - wired straight onto the session
    // instead of going through a NetworkBroadcaster/EventBus topic. Always
    // non-null here: attach() only runs for a room that already exists in
    // the registry (this constructor's own loop, or CreateRoomHandler/
    // FindGameHandler calling this right after registry_.createRoom).
    if (GameSession* session = registry_.room(roomName))
        session->setSnapshotSender(forwardToRoom);

    std::vector<std::unique_ptr<NetworkBroadcaster>> roomBroadcasters;
    roomBroadcasters.push_back(std::make_unique<NetworkBroadcaster>(bus_, GameSession::moveLogTopic(roomName), forwardToRoom));
    roomBroadcasters.push_back(std::make_unique<NetworkBroadcaster>(bus_, GameSession::captureTopic(roomName), forwardToRoom));
    broadcasters_[roomName] = std::move(roomBroadcasters);
}
