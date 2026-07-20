#include "../doctest.h"
#include "../server/app/handlers/LoginHandler.h"
#include "../server/db/UserRepository.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include <memory>

namespace {
    LoginHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("LoginHandler - התחברות ראשונה מצליחה עם ELO התחלתי") {
    UserRepository repo(":memory:");
    ClientSessionRegistry sessions;
    LoginHandler handler(repo, sessions);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    nlohmann::json reply = handler.handle(hdlA, { {"username", "alice"}, {"password", "secret"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("elo").get<int>() == 1200);
}

TEST_CASE("LoginHandler - התחברות מוצלחת רושמת session ב-ClientSessionRegistry") {
    UserRepository repo(":memory:");
    ClientSessionRegistry sessions;
    LoginHandler handler(repo, sessions);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    handler.handle(hdlA, { {"username", "alice"}, {"password", "secret"} });

    const ClientSession* session = sessions.sessionFor(hdlA);
    REQUIRE(session != nullptr);
    CHECK(session->username == "alice");
    CHECK(session->elo == 1200);
}

TEST_CASE("LoginHandler - סיסמה שגויה נכשלת ולא רושמת session") {
    UserRepository repo(":memory:");
    ClientSessionRegistry sessions;
    LoginHandler handler(repo, sessions);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    handler.handle(hdlA, { {"username", "alice"}, {"password", "secret"} });

    std::shared_ptr<int> b;
    auto hdlB = handleFrom(b);
    nlohmann::json reply = handler.handle(hdlB, { {"username", "alice"}, {"password", "wrong"} });

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(sessions.sessionFor(hdlB) == nullptr);
}
