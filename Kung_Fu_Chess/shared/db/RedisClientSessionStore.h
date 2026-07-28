#pragma once
#include "IClientSessionStore.h"
#include <mutex>
#include <string>

struct redisContext;

// Redis-backed IClientSessionStore - Docker/Linux build only (constructed
// by both the API Gateway, to write, and each game-server shard, to read -
// see IClientSessionStore.h for why both sides need it). Stores a small
// JSON blob under a "session:" key with a TTL, the same way
// RedisRoomDirectory stores room ownership under "room:" - both are plain
// key/value use of Redis, no pub/sub.
class RedisClientSessionStore : public IClientSessionStore {
public:
    // Throws std::runtime_error if the connection fails.
    RedisClientSessionStore(const std::string& host, int port);
    ~RedisClientSessionStore() override;

    RedisClientSessionStore(const RedisClientSessionStore&) = delete;
    RedisClientSessionStore& operator=(const RedisClientSessionStore&) = delete;

    void put(const std::string& token, const std::string& username, int elo) override;
    std::optional<ClientSessionData> get(const std::string& token) const override;

private:
    redisContext* ctx_ = nullptr;
    mutable std::mutex mutex_;
};
