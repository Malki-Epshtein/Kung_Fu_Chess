#include "RedisReconnectStore.h"
#include <hiredis/hiredis.h>
#include <stdexcept>

namespace {
    std::string reconnectKey(const std::string& roomName) { return "reconnect:" + roomName; }
}

RedisReconnectStore::RedisReconnectStore(const std::string& host, int port) {
    ctx_ = redisConnect(host.c_str(), port);
    if (!ctx_ || ctx_->err) {
        std::string message = ctx_ ? "Cannot connect to Redis: " + std::string(ctx_->errstr)
                                    : "Cannot allocate Redis context";
        if (ctx_)
            redisFree(ctx_);
        throw std::runtime_error(message);
    }
}

RedisReconnectStore::~RedisReconnectStore() {
    if (ctx_)
        redisFree(ctx_);
}

void RedisReconnectStore::setDisconnected(const std::string& roomName, const std::string& color,
                                           const std::string& username, int elo, int secondsRemaining) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string eloStr = std::to_string(elo);
    std::string secondsStr = std::to_string(secondsRemaining);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "HSET %s color %s username %s elo %s secondsRemaining %s",
        reconnectKey(roomName).c_str(), color.c_str(), username.c_str(), eloStr.c_str(), secondsStr.c_str()));
    if (reply)
        freeReplyObject(reply);
}

void RedisReconnectStore::clearDisconnected(const std::string& roomName) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", reconnectKey(roomName).c_str()));
    if (reply)
        freeReplyObject(reply);
}
