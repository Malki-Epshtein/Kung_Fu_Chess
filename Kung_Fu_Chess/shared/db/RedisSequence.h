#pragma once
#include <mutex>
#include <string>

struct redisContext;

// A single Redis INCR counter - used by the Game Allocator to mint
// globally-unique matchmade room names ("match-1", "match-2", ...) across
// every shard, fixing the bug the old in-process FindGameHandler had
// (`nextMatchId_` was a per-process counter, so two shards both minted
// "match-1" and collided in the room directory). Docker/Linux build only,
// same shape as RedisRoomDirectory/RedisClientSessionStore.
class RedisSequence {
public:
    // Throws std::runtime_error if the connection fails.
    RedisSequence(const std::string& host, int port);
    ~RedisSequence();

    RedisSequence(const RedisSequence&) = delete;
    RedisSequence& operator=(const RedisSequence&) = delete;

    // Atomically increments `key` and returns the new value (starts at 1).
    long long next(const std::string& key);

private:
    redisContext* ctx_ = nullptr;
    mutable std::mutex mutex_;
};
