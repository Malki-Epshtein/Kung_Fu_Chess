#include "LoginFlow.h"
#include "../../shared/protocol/Message.h"
#include "../../shared/protocol/MessageCodec.h"
#include <condition_variable>
#include <mutex>
#include <optional>

namespace {
    constexpr int kMaxAttempts = 3;

    void waitForConnectionOpen(WsClient& client, std::ostream& out) {
        std::mutex              mutex;
        std::condition_variable cv;
        bool                    opened = false;

        client.setOnOpen([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            opened = true;
            cv.notify_one();
        });

        out << "Connecting to server..." << std::endl;
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return opened; });
    }

    LoginResult sendLoginAndWaitForReply(WsClient& client, const std::string& username, const std::string& password) {
        std::mutex                 mutex;
        std::condition_variable    cv;
        std::optional<std::string> replyText;

        client.setOnMessage([&](const std::string& text) {
            std::lock_guard<std::mutex> lock(mutex);
            replyText = text;
            cv.notify_one();
        });

        Message request{ MessageType::Login, { {"username", username}, {"password", password} } };
        client.send(MessageCodec::encode(request));

        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return replyText.has_value(); });

        nlohmann::json j = nlohmann::json::parse(*replyText);
        LoginResult result;
        result.success = j.value("success", false);
        result.message = j.value("message", "");
        result.elo     = j.value("elo", 0);
        return result;
    }
}

LoginResult runLoginFlow(WsClient& client, std::istream& in, std::ostream& out) {
    waitForConnectionOpen(client, out);

    for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
        std::string username, password;
        out << "Username: ";
        in >> username;
        out << "Password: ";
        in >> password;

        LoginResult result = sendLoginAndWaitForReply(client, username, password);
        if (result.success) {
            out << "Login successful (ELO " << result.elo << ")" << std::endl;
            return result;
        }

        out << "Login failed: " << result.message << std::endl;
        if (attempt < kMaxAttempts)
            out << (kMaxAttempts - attempt) << " attempt(s) remaining." << std::endl;
    }

    out << "Too many failed login attempts." << std::endl;
    return LoginResult{};
}
