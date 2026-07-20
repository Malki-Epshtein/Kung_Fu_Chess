#include "NetworkBroadcaster.h"
#include "GameSession.h"

NetworkBroadcaster::NetworkBroadcaster(EventBus& bus, Sender sender) : sender_(std::move(sender)) {
    bus.subscribe(GameSession::kSnapshotTopic, [this](const nlohmann::json& data) {
        sender_(data.dump());
    });
}
