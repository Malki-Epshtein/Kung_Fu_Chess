#pragma once
#include "IReconnectStore.h"
#include <mutex>

struct redisContext;

// Redis-backed IReconnectStore - Docker/Linux build only (never compiled
// into the Windows/MSBuild build - not referenced by any .vcxproj, same as
// RedisRoomDirectory/RedisSequence). Only ever constructed by
// server_main.cpp (when REDIS_HOST is set) and handed to SessionRegistry -
// no gateway or other service needs this.
class RedisReconnectStore : public IReconnectStore {
public:
    // Throws std::runtime_error if the connection fails.
    RedisReconnectStore(const std::string& host, int port);
    ~RedisReconnectStore() override;

    RedisReconnectStore(const RedisReconnectStore&) = delete;
    RedisReconnectStore& operator=(const RedisReconnectStore&) = delete;

    void setDisconnected(const std::string& roomName, const std::string& color,
                          const std::string& username, int elo, int secondsRemaining) override;
    void clearDisconnected(const std::string& roomName) override;

private:
    redisContext* ctx_ = nullptr;

    // Same reasoning as every other Redis-backed class this session
    // (RedisSequence/RedisMatchPool) - a raw hiredis connection isn't
    // thread-safe, and this fires once a second from whichever thread
    // SessionRegistry's disconnect-countdown timer runs on.
    mutable std::mutex mutex_;
};
