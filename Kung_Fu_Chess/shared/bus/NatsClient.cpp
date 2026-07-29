#include "NatsClient.h"
#include <nats/nats.h>
#include <stdexcept>

namespace {
    void onMessage(natsConnection* /*nc*/, natsSubscription* /*sub*/, natsMsg* msg, void* closure) {
        auto* handler = static_cast<INatsClient::MessageHandler*>(closure);
        try {
            nlohmann::json payload = nlohmann::json::parse(
                natsMsg_GetData(msg), natsMsg_GetData(msg) + natsMsg_GetDataLength(msg));
            (*handler)(payload);
        } catch (const std::exception&) {
            // Malformed payload - drop it rather than crash a long-lived service.
        }
        natsMsg_Destroy(msg);
    }

    void onRequest(natsConnection* nc, natsSubscription* /*sub*/, natsMsg* msg, void* closure) {
        auto* handler = static_cast<INatsClient::RequestHandler*>(closure);
        const char* replySubject = natsMsg_GetReply(msg);
        if (replySubject) {
            nlohmann::json replyPayload;
            try {
                nlohmann::json payload = nlohmann::json::parse(
                    natsMsg_GetData(msg), natsMsg_GetData(msg) + natsMsg_GetDataLength(msg));
                replyPayload = (*handler)(payload);
            } catch (const std::exception& e) {
                replyPayload = { {"success", false}, {"message", e.what()} };
            }
            std::string body = replyPayload.dump();
            natsConnection_Publish(nc, replySubject, body.data(), static_cast<int>(body.size()));
        }
        natsMsg_Destroy(msg);
    }
}

NatsClient::NatsClient(const std::string& url) {
    natsStatus status = natsConnection_ConnectTo(&conn_, url.c_str());
    if (status != NATS_OK)
        throw std::runtime_error("Cannot connect to NATS: " + std::string(natsStatus_GetText(status)));
}

NatsClient::~NatsClient() {
    for (natsSubscription* sub : subs_)
        natsSubscription_Destroy(sub);
    if (conn_)
        natsConnection_Destroy(conn_);
}

void NatsClient::publish(const std::string& subject, const nlohmann::json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string body = payload.dump();
    natsConnection_Publish(conn_, subject.c_str(), body.data(), static_cast<int>(body.size()));
}

void NatsClient::subscribe(const std::string& subject, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    messageHandlerStorage_.push_back(std::make_unique<MessageHandler>(std::move(handler)));
    natsSubscription* sub = nullptr;
    natsConnection_Subscribe(&sub, conn_, subject.c_str(), onMessage, messageHandlerStorage_.back().get());
    subs_.push_back(sub);
}

void NatsClient::subscribeQueue(const std::string& subject, const std::string& queueGroup,
                                 MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    messageHandlerStorage_.push_back(std::make_unique<MessageHandler>(std::move(handler)));
    natsSubscription* sub = nullptr;
    natsConnection_QueueSubscribe(&sub, conn_, subject.c_str(), queueGroup.c_str(), onMessage,
                                   messageHandlerStorage_.back().get());
    subs_.push_back(sub);
}

void NatsClient::subscribeRequest(const std::string& subject, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    requestHandlerStorage_.push_back(std::make_unique<RequestHandler>(std::move(handler)));
    natsSubscription* sub = nullptr;
    natsConnection_Subscribe(&sub, conn_, subject.c_str(), onRequest, requestHandlerStorage_.back().get());
    subs_.push_back(sub);
}

void NatsClient::subscribeRequestQueue(const std::string& subject, const std::string& queueGroup,
                                        RequestHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    requestHandlerStorage_.push_back(std::make_unique<RequestHandler>(std::move(handler)));
    natsSubscription* sub = nullptr;
    natsConnection_QueueSubscribe(&sub, conn_, subject.c_str(), queueGroup.c_str(), onRequest,
                                   requestHandlerStorage_.back().get());
    subs_.push_back(sub);
}

std::optional<nlohmann::json> NatsClient::request(const std::string& subject,
                                                     const nlohmann::json& payload,
                                                     int timeoutMs) {
    std::string body = payload.dump();
    natsMsg* reply = nullptr;
    natsStatus status;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status = natsConnection_Request(&reply, conn_, subject.c_str(), body.data(),
                                          static_cast<int>(body.size()), timeoutMs);
    }
    if (status != NATS_OK)
        return std::nullopt;

    std::optional<nlohmann::json> result;
    try {
        result = nlohmann::json::parse(natsMsg_GetData(reply), natsMsg_GetData(reply) + natsMsg_GetDataLength(reply));
    } catch (const std::exception&) {
        result = std::nullopt;
    }
    natsMsg_Destroy(reply);
    return result;
}
