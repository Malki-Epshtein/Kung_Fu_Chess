#include "RedisSequence.h"
#include <hiredis/hiredis.h>
#include <stdexcept>

RedisSequence::RedisSequence(const std::string& host, int port) {
    ctx_ = redisConnect(host.c_str(), port);
    if (!ctx_ || ctx_->err) {
        std::string message = ctx_ ? "Cannot connect to Redis: " + std::string(ctx_->errstr)
                                    : "Cannot allocate Redis context";
        if (ctx_)
            redisFree(ctx_);
        throw std::runtime_error(message);
    }
}

RedisSequence::~RedisSequence() {
    if (ctx_)
        redisFree(ctx_);
}

long long RedisSequence::next(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "INCR %s", key.c_str()));
    if (!reply)
        throw std::runtime_error("RedisSequence::next: no reply");
    long long value = reply->type == REDIS_REPLY_INTEGER ? reply->integer : 0;
    freeReplyObject(reply);
    return value;
}
