#include "../doctest.h"
#include "../server/app/handlers/FindGameHandler.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../server/app/logic/Matchmaker.h"
#include "../shared/protocol/MoveLogCodec.h"
#include <asio/io_context.hpp>
#include <memory>
#include <vector>

namespace {
    FindGameHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }

    // Bundles everything FindGameHandler needs so each TEST_CASE can build
    // one with a single call, same shape as WsServer::run's own wiring.
    struct Fixture {
        SessionRegistry        registry;
        EventBus                bus;
        ClientSessionRegistry  sessions;
        Matchmaker              matchmaker;
        asio::io_context        io;
        std::vector<std::string> attachedRooms;
        std::vector<std::string> pushed;
        FindGameHandler          handler;

        Fixture()
            : handler(sessions, matchmaker, registry, bus,
                      [this](const std::string& name) { attachedRooms.push_back(name); },
                      [this](FindGameHandler::ConnectionHandle, const std::string& text) { pushed.push_back(text); },
                      io) {}
    };
}

TEST_CASE("FindGameHandler - חיבור שלא התחבר נכשל") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = f.handler.handle(hdlA, {});

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(f.pushed.empty());
}

TEST_CASE("FindGameHandler - שחקן ראשון בלי יריב זמין נכנס לרשימת המתנה") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    f.sessions.onLogin(hdlA, "alice", 1200);

    nlohmann::json reply = f.handler.handle(hdlA, {});

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("message").get<std::string>() == "searching for opponent");
    CHECK(f.matchmaker.isWaiting(hdlA));
}

TEST_CASE("FindGameHandler - שני שחקנים בטווח ELO משודכים לאותו חדר") {
    Fixture f;
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    f.sessions.onLogin(hdlA, "alice", 1200);
    f.sessions.onLogin(hdlB, "bob", 1250);

    f.handler.handle(hdlA, {}); // alice waits first -> becomes White
    nlohmann::json reply = f.handler.handle(hdlB, {}); // bob matches -> Black

    CHECK(reply.at("type").get<std::string>() == "GAME_FOUND");
    nlohmann::json payload = reply.at("payload");
    CHECK(payload.at("success").get<bool>());
    CHECK(payload.at("role").get<std::string>() == "Black");

    std::string roomName = payload.at("roomName").get<std::string>();
    REQUIRE(f.registry.roomExists(roomName));
    CHECK(f.registry.roleOf(hdlA) == Chess::Color::White);
    CHECK(f.registry.roleOf(hdlB) == Chess::Color::Black);
    CHECK_FALSE(f.matchmaker.isWaiting(hdlA));

    REQUIRE(f.attachedRooms.size() == 1);
    CHECK(f.attachedRooms[0] == roomName);

    REQUIRE(f.pushed.size() == 1); // alice (the one who was waiting) gets the push
    nlohmann::json pushedMsg = nlohmann::json::parse(f.pushed[0]);
    CHECK(pushedMsg.at("payload").at("role").get<std::string>() == "White");
    CHECK(pushedMsg.at("payload").at("roomName").get<std::string>() == roomName);

    // Freshly matched room -> always empty, but present for shape
    // consistency with JoinRoom's reply (see GameSession::fullMoveLog).
    REQUIRE(payload.contains("moveLog"));
    CHECK(MoveLogCodec::decodeAll(payload.at("moveLog")).white.empty());
    REQUIRE(pushedMsg.at("payload").contains("moveLog"));
    CHECK(MoveLogCodec::decodeAll(pushedMsg.at("payload").at("moveLog")).white.empty());
}

TEST_CASE("FindGameHandler - שחקנים מחוץ לטווח ELO נשארים שניהם ממתינים") {
    Fixture f;
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    f.sessions.onLogin(hdlA, "alice", 900);
    f.sessions.onLogin(hdlB, "bob", 1200); // outside the +-100 range

    f.handler.handle(hdlA, {});
    nlohmann::json reply = f.handler.handle(hdlB, {});

    CHECK(reply.at("message").get<std::string>() == "searching for opponent");
    CHECK(f.matchmaker.isWaiting(hdlA));
    CHECK(f.matchmaker.isWaiting(hdlB));
}
