#pragma once
#include "json.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// Minimal internal publish/subscribe mechanism - a generalization of the
// existing typed Observer interfaces (CaptureObserver, MoveObserver, ...)
// into one generic topic/handler pair. Kept deliberately small: just what
// Stage C's NetworkBroadcaster needs. Extend when a second real subscriber
// (results, sound, animations) actually arrives - don't guess its shape now.
class EventBus {
public:
    using Handler = std::function<void(const nlohmann::json&)>;

    void subscribe(const std::string& topic, Handler handler);
    void publish(const std::string& topic, const nlohmann::json& data) const;

private:
    std::unordered_map<std::string, std::vector<Handler>> handlers;
};
