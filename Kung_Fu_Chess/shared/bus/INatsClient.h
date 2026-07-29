#pragma once
#include "json.hpp"
#include <functional>
#include <optional>
#include <string>

// Cross-process publish/subscribe bus (NATS) - the network-reachable
// analogue of the in-process EventBus, letting the gateway/matchmaker/
// allocator/shards react to the same events without polling each other or
// sharing memory (Server_Design.md's own framing). A null INatsClient*
// (default everywhere, including every existing test and the native
// Windows build) makes publish/subscribe no-ops - same pattern as
// IRoomDirectory.
class INatsClient {
public:
    using MessageHandler = std::function<void(const nlohmann::json&)>;

    // Handles a request and returns the reply payload - used by services
    // that answer NATS request/reply calls (e.g. a shard answering
    // "allocate a room here").
    using RequestHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    virtual ~INatsClient() = default;

    virtual void publish(const std::string& subject, const nlohmann::json& payload) = 0;

    // Fire-and-forget subscription - handler runs once per message,
    // asynchronously, for the life of the process.
    virtual void subscribe(const std::string& subject, MessageHandler handler) = 0;

    // Same as subscribe(), except every process that calls this with the
    // same (subject, queueGroup) pair forms one logical group - NATS
    // delivers each message to exactly one member of the group, not to all
    // of them. This is what makes running N replicas of a service safe
    // when they'd otherwise all independently react to the same message
    // (see Matchmaker's use of this on matchmaking.request - two replicas
    // both handling the same match request could each find a different
    // opponent and publish two conflicting matchmaking.matched events for
    // it). Plain subscribe() is still correct for anything idempotent
    // (e.g. matchmaking.cancel - a second removal of an already-gone
    // ticket is a no-op).
    virtual void subscribeQueue(const std::string& subject, const std::string& queueGroup,
                                 MessageHandler handler) = 0;

    // Subscribes and automatically publishes handler's return value back to
    // the request's reply subject.
    virtual void subscribeRequest(const std::string& subject, RequestHandler handler) = 0;

    // Same relationship subscribeQueue has to subscribe() - queueGroup
    // members split the request traffic instead of each answering every
    // request. Needed for a service that answers NATS requests (like
    // subscribeRequest) but runs multiple replicas (like GameAllocator) -
    // without this, every replica would independently reply to the same
    // request, and the caller only ever sees whichever reply wins the
    // race while every replica's own side effects (e.g. allocating a real
    // room) still happened.
    virtual void subscribeRequestQueue(const std::string& subject, const std::string& queueGroup,
                                        RequestHandler handler) = 0;

    // Synchronous request/reply, with a timeout in milliseconds. Returns
    // nullopt on timeout or if no responder is subscribed.
    virtual std::optional<nlohmann::json> request(const std::string& subject,
                                                    const nlohmann::json& payload,
                                                    int timeoutMs) = 0;
};
