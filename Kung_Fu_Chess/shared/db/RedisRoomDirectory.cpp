#include "RedisRoomDirectory.h"
#include <hiredis/hiredis.h>
#include <stdexcept>

RedisRoomDirectory::RedisRoomDirectory(const std::string& host, int port) {
    ctx_ = redisConnect(host.c_str(), port);
    if (!ctx_ || ctx_->err) {
        std::string message = ctx_ ? "Cannot connect to Redis: " + std::string(ctx_->errstr)
                                    : "Cannot allocate Redis context";
        if (ctx_)
            redisFree(ctx_);
        throw std::runtime_error(message);
    }
}

RedisRoomDirectory::~RedisRoomDirectory() {
    if (ctx_)
        redisFree(ctx_);
}

void RedisRoomDirectory::set(const std::string& roomName, const std::string& shardAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SET room:%s %s", roomName.c_str(), shardAddress.c_str()));
    if (reply)
        freeReplyObject(reply);
}

void RedisRoomDirectory::erase(const std::string& roomName) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL room:%s", roomName.c_str()));
    if (reply)
        freeReplyObject(reply);
}

std::string RedisRoomDirectory::get(const std::string& roomName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "GET room:%s", roomName.c_str()));
    if (!reply)
        return "";
    std::string result;
    if (reply->type == REDIS_REPLY_STRING)
        result.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return result;
}

std::vector<std::string> RedisRoomDirectory::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "KEYS room:*"));
    std::vector<std::string> names;
    if (!reply)
        return names;
    if (reply->type == REDIS_REPLY_ARRAY) {
        static constexpr size_t kPrefixLen = 5; // "room:"
        names.reserve(reply->elements);
        for (size_t i = 0; i < reply->elements; ++i) {
            redisReply* element = reply->element[i];
            if (element->type == REDIS_REPLY_STRING && element->len > kPrefixLen)
                names.emplace_back(element->str + kPrefixLen, element->len - kPrefixLen);
        }
    }
    freeReplyObject(reply);
    return names;
}
