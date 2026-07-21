#include "LoginFlow.h"
#include "../net/BlockingRequest.h"
#include "../../shared/protocol/Message.h"
#include <condition_variable>
#include <mutex>

namespace {
    constexpr int kMaxAttempts = 3;

    void waitForConnectionOpen(WsClient& client, std::ostream& out) {
        // Same reasoning as sendAndWaitForReply (BlockingRequest.cpp):
        // nothing ever calls setOnOpen again after this, so this handler
        // stays the installed one for the rest of the connection's life -
        // it must not capture references to this function's own locals,
        // which are destroyed the moment this function returns.
        auto mutex  = std::make_shared<std::mutex>();
        auto cv     = std::make_shared<std::condition_variable>();
        auto opened = std::make_shared<bool>(false);

        client.setOnOpen([mutex, cv, opened]() {
            std::lock_guard<std::mutex> lock(*mutex);
            *opened = true;
            cv->notify_one();
        });

        out << "Connecting to server..." << std::endl;
        std::unique_lock<std::mutex> lock(*mutex);
        cv->wait(lock, [&] { return *opened; });
    }

    LoginResult sendLoginAndWaitForReply(WsClient& client, const std::string& username, const std::string& password) {
        Message request{ MessageType::Login, { {"username", username}, {"password", password} } };
        nlohmann::json j = sendAndWaitForReply(client, request);

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
