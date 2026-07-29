#include "ShardRegistry.h"

ShardRegistry::ShardRegistry(INatsClient& nats, int staleAfterSeconds) : staleAfterSeconds_(staleAfterSeconds) {
    nats.subscribe("shard.heartbeat", [this](const nlohmann::json& event) {
        std::string shard = event.at("shard").get<std::string>();
        std::lock_guard<std::mutex> lock(mutex_);
        lastSeen_[shard] = std::chrono::steady_clock::now();
    });
}

std::vector<std::string> ShardRegistry::liveShards() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> live;
    for (auto it = lastSeen_.begin(); it != lastSeen_.end(); ) {
        auto ageSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
        if (ageSeconds > staleAfterSeconds_) {
            it = lastSeen_.erase(it);
        } else {
            live.push_back(it->first);
            ++it;
        }
    }
    return live;
}
