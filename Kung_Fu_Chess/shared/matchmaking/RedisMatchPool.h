#pragma once
#include <mutex>
#include <optional>
#include <string>

struct redisContext;

// A ticket popped out of the pool by findMatch() - everything
// matchmaker_main.cpp needs to publish matchmaking.matched, without having
// to keep its own per-replica record of a ticket some *other* replica
// originally added (see findMatch's comment for why that matters).
struct MatchCandidate {
    std::string ticketId;
    std::string username;
    int         elo = 0;
};

// Redis-backed replacement for Matchmaker's in-memory pool_ (see
// Matchmaker.h) - a shared waiting pool every Matchmaker replica reads/
// writes, instead of one private to a single process. Docker/Linux build
// only, same shape as RedisSequence - never referenced by any .vcxproj.
//
// Two structures per ticket: a sorted set ("matchpool", member = ticketId,
// score = elo) for the ELO-range query, and a hash ("ticket:<id>") holding
// the username/elo that query alone can't carry (a Redis member is just an
// opaque string). findMatch() is the one operation that has to be atomic -
// two Matchmaker replicas racing on it must never both succeed against the
// same opponent - done as a single Lua EVAL (find, remove, and read the
// matched ticket's info in one server-side round trip) rather than a
// WATCH/MULTI retry loop.
class RedisMatchPool {
public:
    // Throws std::runtime_error if the connection fails.
    RedisMatchPool(const std::string& host, int port);
    ~RedisMatchPool();

    RedisMatchPool(const RedisMatchPool&) = delete;
    RedisMatchPool& operator=(const RedisMatchPool&) = delete;

    void addToPool(const std::string& ticketId, const std::string& username, int elo);

    // No-op if ticketId isn't in the pool - used both when a match is
    // found and when a waiting ticket is cancelled/times out. Idempotent
    // (ZREM/DEL on an already-gone key are no-ops), so this is safe to call
    // from every replica without coordination - see matchmaker_main.cpp's
    // matchmaking.cancel handler, which stays a plain fan-out subscribe for
    // exactly this reason.
    void remove(const std::string& ticketId);

    // Atomically finds any waiting ticket within [elo-eloRange,
    // elo+eloRange], removes it, and returns its username/elo - nullopt if
    // none qualifies. Safe to call concurrently from multiple Matchmaker
    // replicas: at most one caller can ever get a given ticket back, even
    // when that ticket was originally addToPool()'d by a *different*
    // replica than the one calling findMatch() now - the whole point of
    // moving this pool to Redis instead of each replica's own memory.
    std::optional<MatchCandidate> findMatch(int elo, int eloRange);

    // True while ticketId is still sitting in the pool.
    bool isWaiting(const std::string& ticketId) const;

private:
    redisContext* ctx_ = nullptr;

    // A raw hiredis connection isn't thread-safe (same reasoning as
    // RedisSequence/RedisRoomDirectory's own mutexes) - matchmaker_main.cpp
    // calls these from both NATS's delivery thread (matchmaking.request/
    // .cancel) and its own per-ticket timeout threads (waitForMatch).
    mutable std::mutex mutex_;
};
