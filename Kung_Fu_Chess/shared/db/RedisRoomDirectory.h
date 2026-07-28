#pragma once
#include "IRoomDirectory.h"
#include <mutex>
#include <string>

struct redisContext;

// Redis-backed IRoomDirectory - Docker/Linux build only (never compiled
// into the Windows/MSBuild build - not referenced by any .vcxproj). Plain
// key/value store (SET/GET/DEL under a "room:" prefix), not pub/sub -
// matches Server_Design.md's own scoping ("a room is fully owned by one
// pod", no broadcast fan-out needed here). Used two ways: a shard
// constructs one to write (server_main.cpp, when REDIS_HOST is set), and
// the Gateway constructs one to read (gateway_main.cpp, same env var) when
// deciding which shard to route a JOIN_ROOM to.
class RedisRoomDirectory : public IRoomDirectory {
public:
    // Throws std::runtime_error if the connection fails.
    RedisRoomDirectory(const std::string& host, int port);
    ~RedisRoomDirectory() override;

    RedisRoomDirectory(const RedisRoomDirectory&) = delete;
    RedisRoomDirectory& operator=(const RedisRoomDirectory&) = delete;

    void set(const std::string& roomName, const std::string& shardAddress) override;
    void erase(const std::string& roomName) override;
    std::string get(const std::string& roomName) const override;
    std::vector<std::string> list() const override;

private:
    redisContext* ctx_ = nullptr;

    // Same reasoning as PostgresUserRepository's mutex: a raw hiredis
    // connection isn't thread-safe. On the shard side, createRoom/leave/
    // reapIfSafe all run on the io_service thread; on the Gateway side,
    // everything runs on its own single io_context thread too - but this
    // stays defensive either way.
    mutable std::mutex mutex_;
};
