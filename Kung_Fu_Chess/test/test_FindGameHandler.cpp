#include "../doctest.h"
#include "../server/app/handlers/FindGameHandler.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../server/app/logic/MatchTicketRegistry.h"
#include "../shared/protocol/MoveLogCodec.h"
#include "FakeNatsClient.h"
#include <asio/io_context.hpp>
#include <memory>
#include <vector>

namespace {
    FindGameHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }

    // Bundles everything FindGameHandler needs, using FakeNatsClient
    // instead of a real broker so the full matchmaking.request/.assigned/
    // .timeout round trip is testable synchronously (see FakeNatsClient.h).
    // "shard" is the ticket-ID prefix MatchTicketRegistry mints with
    // (see its header) - tests below rely on the exact "shard#1" format to
    // build the matchmaking.assigned/.timeout events a real Matchmaker/
    // GameAllocator service would have published.
    struct Fixture {
        FakeNatsClient          nats;
        ClientSessionRegistry  sessions;
        MatchTicketRegistry      tickets{ "shard" };
        asio::io_context          io;
        std::vector<std::string> pushed;
        FindGameHandler           handler;

        Fixture()
            : handler(sessions, tickets, &nats,
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

TEST_CASE("FindGameHandler - ללא NATS מחזיר matchmaking unavailable") {
    ClientSessionRegistry sessions;
    MatchTicketRegistry tickets{ "shard" };
    asio::io_context io;
    FindGameHandler handler(sessions, tickets, nullptr, [](FindGameHandler::ConnectionHandle, const std::string&) {}, io);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    sessions.onLogin(hdlA, "alice", 1200);

    nlohmann::json reply = handler.handle(hdlA, {});

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(reply.at("message").get<std::string>() == "matchmaking unavailable");
}

TEST_CASE("FindGameHandler - שחקן מפרסם matchmaking.request ונכנס להמתנה") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    f.sessions.onLogin(hdlA, "alice", 1200);

    nlohmann::json published;
    f.nats.subscribe("matchmaking.request", [&](const nlohmann::json& payload) { published = payload; });

    nlohmann::json reply = f.handler.handle(hdlA, {});

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("message").get<std::string>() == "searching for opponent");
    CHECK(published.at("ticketId").get<std::string>() == "shard#1");
    CHECK(published.at("username").get<std::string>() == "alice");
    CHECK(published.at("elo").get<int>() == 1200);
}

TEST_CASE("FindGameHandler - matchmaking.assigned עבור טיקט של החיבור הזה נדחף כ-GAME_FOUND") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    f.sessions.onLogin(hdlA, "alice", 1200);
    f.handler.handle(hdlA, {}); // mints ticket "shard#1"

    f.nats.publish("matchmaking.assigned",
                    { {"ticketA", "shard#1"}, {"roleA", "White"},
                      {"ticketB", "other-shard#7"}, {"roleB", "Black"},
                      {"roomName", "match-1"}, {"shard", "ws://gameserver-a:9002"},
                      {"whiteName", "alice"}, {"whiteElo", 1200},
                      {"blackName", "bob"}, {"blackElo", 1250} });
    f.io.poll(); // the subscription marshals the actual push through io_context::post

    REQUIRE(f.pushed.size() == 1);
    nlohmann::json pushedMsg = nlohmann::json::parse(f.pushed[0]);
    CHECK(pushedMsg.at("type").get<std::string>() == "GAME_FOUND");
    nlohmann::json payload = pushedMsg.at("payload");
    CHECK(payload.at("success").get<bool>());
    CHECK(payload.at("role").get<std::string>() == "White");
    CHECK(payload.at("roomName").get<std::string>() == "match-1");
    CHECK(payload.at("shard").get<std::string>() == "ws://gameserver-a:9002");
    CHECK(payload.at("whiteName").get<std::string>() == "alice");
    CHECK(payload.at("blackName").get<std::string>() == "bob");

    // Regression check: a bare `[]` here (instead of the
    // {"white":[...],"black":[...]} shape MoveLogCodec::encodeAll produces)
    // compiles and passes every other assertion above, but crashes the
    // real client with "cannot use at() with array" the moment it calls
    // MoveLogCodec::decodeAll on this field - only caught by an actual
    // two-client run, not by asserting individual keys, so decode it here.
    REQUIRE(payload.contains("moveLog"));
    CHECK(MoveLogCodec::decodeAll(payload.at("moveLog")).white.empty());
    CHECK(MoveLogCodec::decodeAll(payload.at("moveLog")).black.empty());
}

TEST_CASE("FindGameHandler - matchmaking.assigned לטיקט של חיבור אחר לא דוחף כלום") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    f.sessions.onLogin(hdlA, "alice", 1200);
    f.handler.handle(hdlA, {}); // mints ticket "shard#1"

    // Neither ticket belongs to this shard's MatchTicketRegistry - the
    // event the OTHER shard's own subscription would react to.
    f.nats.publish("matchmaking.assigned",
                    { {"ticketA", "other-shard#3"}, {"roleA", "White"},
                      {"ticketB", "other-shard#9"}, {"roleB", "Black"},
                      {"roomName", "match-2"}, {"shard", "ws://gameserver-b:9002"},
                      {"whiteName", "carol"}, {"whiteElo", 1000},
                      {"blackName", "dave"}, {"blackElo", 1050} });
    f.io.poll();

    CHECK(f.pushed.empty());
}

TEST_CASE("FindGameHandler - matchmaking.timeout עבור טיקט ממתין נדחף כ-GAME_FOUND כושל") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    f.sessions.onLogin(hdlA, "alice", 1200);
    f.handler.handle(hdlA, {}); // mints ticket "shard#1"

    f.nats.publish("matchmaking.timeout", { {"ticketId", "shard#1"} });
    f.io.poll();

    REQUIRE(f.pushed.size() == 1);
    nlohmann::json pushedMsg = nlohmann::json::parse(f.pushed[0]);
    CHECK(pushedMsg.at("type").get<std::string>() == "GAME_FOUND");
    CHECK_FALSE(pushedMsg.at("payload").at("success").get<bool>());
}
