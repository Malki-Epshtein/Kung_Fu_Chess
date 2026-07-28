#include "LoginFlow.h"

namespace {
    constexpr int kMaxAttempts = 3;

    LoginResult postLogin(HttpClient& api, const std::string& username, const std::string& password) {
        nlohmann::json reply = api.post("/login", { {"username", username}, {"password", password} });

        LoginResult result;
        result.success = reply.value("success", false);
        result.message = reply.value("message", "");
        result.elo     = reply.value("elo", 0);
        result.token   = reply.value("token", "");
        return result;
    }
}

LoginResult runLoginFlow(HttpClient& api, std::istream& in, std::ostream& out) {
    for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
        std::string username, password;
        out << "Username: ";
        in >> username;
        out << "Password: ";
        in >> password;

        LoginResult result = postLogin(api, username, password);
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
