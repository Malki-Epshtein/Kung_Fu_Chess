#pragma once
#include "INatsClient.h"
#include <memory>
#include <mutex>
#include <vector>

// Must match cnats' own opaque-type declarations exactly (nats.h:
// `typedef struct __natsConnection natsConnection;`) - a plain
// `struct natsConnection;` forward-declares a DIFFERENT, incompatible tag
// and fails to compile once nats.h is included in the .cpp.
typedef struct __natsConnection natsConnection;
typedef struct __natsSubscription natsSubscription;

// cnats-backed INatsClient - Docker/Linux build only (never compiled into
// the Windows/MSBuild build - not referenced by any .vcxproj). Each
// subscribe()/subscribeRequest() spawns its own cnats delivery thread (the
// library's own async model); handlers run on that thread, so publish()/
// request() stay mutex-guarded for the same reason RedisRoomDirectory's are
// - a raw connection handle isn't thread-safe to share unguarded.
class NatsClient : public INatsClient {
public:
    // Throws std::runtime_error if the connection fails.
    explicit NatsClient(const std::string& url);
    ~NatsClient() override;

    NatsClient(const NatsClient&) = delete;
    NatsClient& operator=(const NatsClient&) = delete;

    void publish(const std::string& subject, const nlohmann::json& payload) override;
    void subscribe(const std::string& subject, MessageHandler handler) override;
    void subscribeQueue(const std::string& subject, const std::string& queueGroup,
                         MessageHandler handler) override;
    void subscribeRequest(const std::string& subject, RequestHandler handler) override;
    void subscribeRequestQueue(const std::string& subject, const std::string& queueGroup,
                                RequestHandler handler) override;
    std::optional<nlohmann::json> request(const std::string& subject,
                                            const nlohmann::json& payload,
                                            int timeoutMs) override;

private:
    natsConnection* conn_ = nullptr;
    mutable std::mutex mutex_;

    // Keeps subscriptions (and their handler closures) alive for the
    // process lifetime - matches this class's only usage pattern (a service
    // subscribes once at startup and never unsubscribes).
    std::vector<natsSubscription*> subs_;
    std::vector<std::unique_ptr<MessageHandler>> messageHandlerStorage_;
    std::vector<std::unique_ptr<RequestHandler>> requestHandlerStorage_;
};
