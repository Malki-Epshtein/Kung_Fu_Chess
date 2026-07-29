#pragma once
#include "../shared/bus/INatsClient.h"
#include <unordered_map>
#include <vector>

// In-process, synchronous test double for INatsClient - publish()
// immediately invokes every handler subscribed to that exact subject (no
// wildcard support, no threading involved), so tests can drive a full
// matchmaking.request -> matchmaking.assigned round trip deterministically
// without a real NATS broker. subscribeRequest()'s handler is invoked
// synchronously by request(), and its return value becomes the "reply".
class FakeNatsClient : public INatsClient {
public:
    void publish(const std::string& subject, const nlohmann::json& payload) override {
        for (auto& handler : subscribers_[subject])
            handler(payload);

        // Queue groups: exactly one handler per (subject, group) sees each
        // message, round-robin across however many are registered - models
        // real NATS's "one member of the group" delivery deterministically
        // enough for tests that want to exercise it.
        auto it = queueSubscribers_.find(subject);
        if (it == queueSubscribers_.end())
            return;
        for (auto& [group, handlers] : it->second) {
            if (handlers.empty())
                continue;
            size_t& next = queueRoundRobin_[subject + "\x1f" + group];
            handlers[next % handlers.size()](payload);
            ++next;
        }
    }

    void subscribe(const std::string& subject, MessageHandler handler) override {
        subscribers_[subject].push_back(std::move(handler));
    }

    void subscribeQueue(const std::string& subject, const std::string& queueGroup,
                         MessageHandler handler) override {
        queueSubscribers_[subject][queueGroup].push_back(std::move(handler));
    }

    void subscribeRequest(const std::string& subject, RequestHandler handler) override {
        requestHandlers_[subject] = std::move(handler);
    }

    void subscribeRequestQueue(const std::string& subject, const std::string& queueGroup,
                                RequestHandler handler) override {
        requestQueueHandlers_[subject][queueGroup].push_back(std::move(handler));
    }

    std::optional<nlohmann::json> request(const std::string& subject, const nlohmann::json& payload,
                                           int /*timeoutMs*/) override {
        auto it = requestHandlers_.find(subject);
        if (it != requestHandlers_.end())
            return it->second(payload);

        // Same round-robin-per-group modeling as publish()'s queue groups
        // above - request()'s subject may only ever be answered by one
        // queue-grouped responder, so there's no "call every group" case
        // to handle here the way publish() has to.
        auto groupsIt = requestQueueHandlers_.find(subject);
        if (groupsIt == requestQueueHandlers_.end() || groupsIt->second.empty())
            return std::nullopt;
        auto& [group, handlers] = *groupsIt->second.begin();
        if (handlers.empty())
            return std::nullopt;
        size_t& next = queueRoundRobin_[subject + "\x1f" + group];
        nlohmann::json reply = handlers[next % handlers.size()](payload);
        ++next;
        return reply;
    }

private:
    std::unordered_map<std::string, std::vector<MessageHandler>> subscribers_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<MessageHandler>>> queueSubscribers_;
    std::unordered_map<std::string, size_t> queueRoundRobin_;
    std::unordered_map<std::string, RequestHandler> requestHandlers_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<RequestHandler>>> requestQueueHandlers_;
};
