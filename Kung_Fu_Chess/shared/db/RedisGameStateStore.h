#pragma once
#include "IGameStateStore.h"
#include <mutex>

struct redisContext;

// Redis-backed IGameStateStore - Docker/Linux build only (never compiled
// into the Windows/MSBuild build - not referenced by any .vcxproj, same as
// RedisReconnectStore/RedisRoomDirectory). Only ever constructed by
// server_main.cpp (when REDIS_HOST is set) and handed to
// GameStateMirrorService - no gateway or other service needs this.
class RedisGameStateStore : public IGameStateStore {
public:
    // Throws std::runtime_error if the connection fails.
    RedisGameStateStore(const std::string& host, int port);
    ~RedisGameStateStore() override;

    RedisGameStateStore(const RedisGameStateStore&) = delete;
    RedisGameStateStore& operator=(const RedisGameStateStore&) = delete;

    void save(const std::string& roomName, const std::string& boardStateJson,
              const std::string& moveLogJson, const std::string& whiteUsername, int whiteElo,
              const std::string& blackUsername, int blackElo) override;
    void clear(const std::string& roomName) override;

private:
    redisContext* ctx_ = nullptr;

    // Same reasoning as every other Redis-backed class this session
    // (RedisSequence/RedisMatchPool/RedisReconnectStore) - a raw hiredis
    // connection isn't thread-safe, and this fires from whichever thread
    // GameSession::publishStep() runs its per-room EventBus subscribers on.
    mutable std::mutex mutex_;
};
