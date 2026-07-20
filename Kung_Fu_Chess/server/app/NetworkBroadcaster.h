#pragma once
#include "../../shared/bus/EventBus.h"
#include <functional>
#include <string>

// Subscribes to GameSession::kSnapshotTopic and forwards each published
// snapshot, serialized as text, to whatever Sender it's given. Knows
// nothing about WebSockets/websocketpp - stays unit-testable with a fake
// sender, and the concrete send-to-all-clients logic lives in WsServer.
class NetworkBroadcaster {
public:
    using Sender = std::function<void(const std::string&)>;

    NetworkBroadcaster(EventBus& bus, Sender sender);

private:
    Sender sender_;
};
