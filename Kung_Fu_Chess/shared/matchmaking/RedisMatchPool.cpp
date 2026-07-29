#include "RedisMatchPool.h"
#include <hiredis/hiredis.h>
#include <stdexcept>

namespace {
    constexpr const char* kPoolKey = "matchpool";
    std::string ticketInfoKey(const std::string& ticketId) { return "ticket:" + ticketId; }

    // Finds any pool member with score in [ARGV[1], ARGV[2]], removes it,
    // and reads back its info hash (then deletes that too) - all in one
    // script. Running this as one atomic step rather than separate
    // ZRANGEBYSCORE/ZREM/HGET/DEL calls is the whole point: Redis executes
    // a script single-threaded, so two callers racing on EVAL can never
    // both walk away with the same member. No '%' characters in here on
    // purpose - it's substituted into a hiredis printf-style command
    // format string below.
    constexpr const char* kFindAndRemoveScript =
        "local found = redis.call('ZRANGEBYSCORE', KEYS[1], ARGV[1], ARGV[2], 'LIMIT', 0, 1) "
        "if #found == 0 then return nil end "
        "local ticketId = found[1] "
        "redis.call('ZREM', KEYS[1], ticketId) "
        "local infoKey = 'ticket:' .. ticketId "
        "local username = redis.call('HGET', infoKey, 'username') "
        "local elo = redis.call('HGET', infoKey, 'elo') "
        "redis.call('DEL', infoKey) "
        "return {ticketId, username, elo}";
}

RedisMatchPool::RedisMatchPool(const std::string& host, int port) {
    ctx_ = redisConnect(host.c_str(), port);
    if (!ctx_ || ctx_->err) {
        std::string message = ctx_ ? "Cannot connect to Redis: " + std::string(ctx_->errstr)
                                    : "Cannot allocate Redis context";
        if (ctx_)
            redisFree(ctx_);
        throw std::runtime_error(message);
    }
}

RedisMatchPool::~RedisMatchPool() {
    if (ctx_)
        redisFree(ctx_);
}

void RedisMatchPool::addToPool(const std::string& ticketId, const std::string& username, int elo) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string eloStr = std::to_string(elo);
    std::string infoKey = ticketInfoKey(ticketId);

    // HSET before ZADD, deliberately - findMatch()'s script becomes able
    // to find this ticket the instant ZADD returns, and it unconditionally
    // reads the info hash right after. Setting the hash first guarantees
    // it's already there by then; the reverse order would leave a real
    // (if narrow) window where a concurrent findMatch() on another replica
    // could pop this ticket before its username/elo were ever written.
    redisReply* hset = static_cast<redisReply*>(redisCommand(
        ctx_, "HSET %s username %s elo %s", infoKey.c_str(), username.c_str(), eloStr.c_str()));
    if (hset)
        freeReplyObject(hset);

    redisReply* zadd = static_cast<redisReply*>(
        redisCommand(ctx_, "ZADD %s %d %s", kPoolKey, elo, ticketId.c_str()));
    if (zadd)
        freeReplyObject(zadd);
}

void RedisMatchPool::remove(const std::string& ticketId) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* zrem = static_cast<redisReply*>(
        redisCommand(ctx_, "ZREM %s %s", kPoolKey, ticketId.c_str()));
    if (zrem)
        freeReplyObject(zrem);

    redisReply* del = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", ticketInfoKey(ticketId).c_str()));
    if (del)
        freeReplyObject(del);
}

std::optional<MatchCandidate> RedisMatchPool::findMatch(int elo, int eloRange) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "EVAL %s 1 %s %d %d", kFindAndRemoveScript, kPoolKey,
                      elo - eloRange, elo + eloRange));
    if (!reply)
        return std::nullopt;

    std::optional<MatchCandidate> result;
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 &&
        reply->element[0]->type == REDIS_REPLY_STRING &&
        reply->element[1]->type == REDIS_REPLY_STRING &&
        reply->element[2]->type == REDIS_REPLY_STRING) {
        MatchCandidate candidate;
        candidate.ticketId = std::string(reply->element[0]->str, reply->element[0]->len);
        candidate.username = std::string(reply->element[1]->str, reply->element[1]->len);
        candidate.elo      = std::stoi(std::string(reply->element[2]->str, reply->element[2]->len));
        result = std::move(candidate);
    }
    freeReplyObject(reply);
    return result;
}

bool RedisMatchPool::isWaiting(const std::string& ticketId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "ZSCORE %s %s", kPoolKey, ticketId.c_str()));
    if (!reply)
        return false;
    bool waiting = reply->type == REDIS_REPLY_STRING;
    freeReplyObject(reply);
    return waiting;
}
