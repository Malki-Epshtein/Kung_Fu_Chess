#include "RedisGameStateStore.h"
#include <hiredis/hiredis.h>
#include <stdexcept>

namespace {
    std::string gameStateKey(const std::string& roomName) { return "gamestate:" + roomName; }
}

RedisGameStateStore::RedisGameStateStore(const std::string& host, int port) {
    ctx_ = redisConnect(host.c_str(), port);
    if (!ctx_ || ctx_->err) {
        std::string message = ctx_ ? "Cannot connect to Redis: " + std::string(ctx_->errstr)
                                    : "Cannot allocate Redis context";
        if (ctx_)
            redisFree(ctx_);
        throw std::runtime_error(message);
    }
}

RedisGameStateStore::~RedisGameStateStore() {
    if (ctx_)
        redisFree(ctx_);
}

void RedisGameStateStore::save(const std::string& roomName, const std::string& boardStateJson,
                                const std::string& moveLogJson, const std::string& whiteUsername, int whiteElo,
                                const std::string& blackUsername, int blackElo) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string whiteEloStr = std::to_string(whiteElo);
    std::string blackEloStr = std::to_string(blackElo);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "HSET %s board %s moveLog %s whiteUsername %s whiteElo %s blackUsername %s blackElo %s",
        gameStateKey(roomName).c_str(), boardStateJson.c_str(), moveLogJson.c_str(),
        whiteUsername.c_str(), whiteEloStr.c_str(), blackUsername.c_str(), blackEloStr.c_str()));
    if (reply)
        freeReplyObject(reply);
}

void RedisGameStateStore::clear(const std::string& roomName) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", gameStateKey(roomName).c_str()));
    if (reply)
        freeReplyObject(reply);
}
