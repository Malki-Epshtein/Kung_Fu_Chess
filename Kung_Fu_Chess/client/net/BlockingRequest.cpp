#include "BlockingRequest.h"
#include "../../shared/protocol/MessageCodec.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>

nlohmann::json sendAndWaitForReply(WsClient& client, const Message& request) {
    // Held by shared_ptr and captured BY VALUE below (not by reference to
    // this function's locals): client.setOnMessage installs this handler
    // and does not restore whatever was there before, so it can still be
    // the active handler for a brief window after this function has
    // already returned (the caller installs its own replacement handler
    // afterward, but not instantly - e.g. a snapshot broadcast can arrive
    // in that gap, now that the room is joined before the reply is even
    // sent). Capturing by reference there caused a real crash: the
    // handler fired after mutex/cv/reply were already destroyed
    // ("unlock of unowned mutex"). shared_ptr keeps them alive for as
    // long as the handler itself is still installed, however long that is.
    auto mutex = std::make_shared<std::mutex>();
    auto cv    = std::make_shared<std::condition_variable>();
    auto reply = std::make_shared<std::optional<nlohmann::json>>();

    client.setOnMessage([mutex, cv, reply](const std::string& text) {
        std::lock_guard<std::mutex> lock(*mutex);
        if (reply->has_value())
            return; // already got our reply - a later stray message (e.g.
                     // the room's first periodic snapshot) is not it
        try {
            *reply = nlohmann::json::parse(text);
        } catch (const std::exception&) {
            return; // malformed text - not a real reply, keep waiting
        }
        cv->notify_all();
    });

    client.send(MessageCodec::encode(request));

    std::unique_lock<std::mutex> lock(*mutex);
    cv->wait(lock, [&] { return reply->has_value(); });
    return **reply;
}
