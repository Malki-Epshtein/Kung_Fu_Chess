#include "RedisClientSessionStore.h"
#include "json.hpp"
#include <hiredis/hiredis.h>
#include <stdexcept>

namespace {
    // Ephemeral on purpose (Server_Design.md: Redis holds "sessions...
    // reconnect state" - not durable identity, that's Postgres's job). An
    // hour is generous for how long a login-to-first-room gap should ever
    // realistically be.
    constexpr int kSessionTtlSeconds = 3600;
}

RedisClientSessionStore::RedisClientSessionStore(const std::string& host, int port) {
    ctx_ = redisConnect(host.c_str(), port);
    if (!ctx_ || ctx_->err) {
        std::string message = ctx_ ? "Cannot connect to Redis: " + std::string(ctx_->errstr)
                                    : "Cannot allocate Redis context";
        if (ctx_)
            redisFree(ctx_);
        throw std::runtime_error(message);
    }
}

RedisClientSessionStore::~RedisClientSessionStore() {
    if (ctx_)
        redisFree(ctx_);
}

void RedisClientSessionStore::put(const std::string& token, const std::string& username, int elo) {
    nlohmann::json blob = { {"username", username}, {"elo", elo} };
    std::string value = blob.dump();

    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SETEX session:%s %d %s", token.c_str(), kSessionTtlSeconds, value.c_str()));
    if (reply)
        freeReplyObject(reply);
}

std::optional<ClientSessionData> RedisClientSessionStore::get(const std::string& token) const {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "GET session:%s", token.c_str()));
    if (!reply)
        return std::nullopt;

    std::optional<ClientSessionData> result;
    if (reply->type == REDIS_REPLY_STRING) {
        try {
            nlohmann::json blob = nlohmann::json::parse(std::string(reply->str, reply->len));
            result = ClientSessionData{ blob.at("username").get<std::string>(), blob.at("elo").get<int>() };
        } catch (const std::exception&) {
            // Corrupt/unexpected value - treat as "no session", not a crash.
        }
    }
    freeReplyObject(reply);
    return result;
}
