#include "EventBus.h"

void EventBus::subscribe(const std::string& topic, Handler handler) {
    handlers[topic].push_back(std::move(handler));
}

void EventBus::publish(const std::string& topic, const nlohmann::json& data) const {
    auto it = handlers.find(topic);
    if (it == handlers.end())
        return;
    for (const auto& handler : it->second)
        handler(data);
}
