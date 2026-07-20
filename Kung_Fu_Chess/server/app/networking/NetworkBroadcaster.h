#pragma once
#include "../../../shared/bus/EventBus.h"
#include <functional>
#include <string>

// Subscribes to one bus topic and forwards each published payload,
// serialized as text, to whatever Sender it's given. Knows nothing about
// WebSockets/websocketpp - stays unit-testable with a fake sender, and the
// concrete send-to-clients logic lives in WsServer. One instance is
// attached per room (topic = GameSession::snapshotTopic(roomName)), so a
// broadcaster only ever forwards its own room's snapshots.
class NetworkBroadcaster {
public:
    using Sender = std::function<void(const std::string&)>;

    NetworkBroadcaster(EventBus& bus, const std::string& topic, Sender sender);

private:
    Sender sender_;
};
