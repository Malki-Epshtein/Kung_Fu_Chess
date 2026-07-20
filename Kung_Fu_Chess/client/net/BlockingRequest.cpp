#include "BlockingRequest.h"
#include "../../shared/protocol/MessageCodec.h"
#include <condition_variable>
#include <mutex>
#include <optional>

nlohmann::json sendAndWaitForReply(WsClient& client, const Message& request) {
    std::mutex                    mutex;
    std::condition_variable       cv;
    std::optional<nlohmann::json> reply;

    client.setOnMessage([&](const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex);
        reply = nlohmann::json::parse(text);
        cv.notify_all();
    });

    client.send(MessageCodec::encode(request));

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return reply.has_value(); });
    return *reply;
}
