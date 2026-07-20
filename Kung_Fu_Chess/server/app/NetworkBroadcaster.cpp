#include "NetworkBroadcaster.h"

NetworkBroadcaster::NetworkBroadcaster(EventBus& bus, const std::string& topic, Sender sender) : sender_(std::move(sender)) {
    bus.subscribe(topic, [this](const nlohmann::json& data) {
        sender_(data.dump());
    });
}
