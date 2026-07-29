#include "../doctest.h"
#include "../server/app/handlers/MessageDispatcher.h"
#include "../server/app/networking/BroadcasterManager.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../server/app/logic/MatchTicketRegistry.h"
#include "../server/app/logic/EloService.h"
#include "../server/app/logic/GameHistoryService.h"
#include "../server/app/logic/GameStateMirrorService.h"
#include "../server/app/logic/StartingBoard.h"
#include "../server/db/UserRepository.h"
#include "../shared/protocol/MessageCodec.h"
#include "FakeNatsClient.h"
#include <asio/io_context.hpp>
#include <memory>
#include <vector>

namespace {
    MessageDispatcher::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }

    // Bundles every MessageDispatcher dependency, same shape as WsServer's
    // own composition - lets each TEST_CASE build one with a single call.
    // `nats` defaults to null (matchmaking unavailable, matching every
    // non-FindGame test's needs unchanged); pass a FakeNatsClient for tests
    // that exercise FIND_GAME. `pushed` records every push (FindGameHandler's
    // GAME_FOUND, in particular), replacing the previous fixed no-op lambda.
    struct Fixture {
        UserRepository          users{ ":memory:" };
        ClientSessionRegistry  clientSessions;
        SessionRegistry          registry;
        EventBus                  bus;
        MatchTicketRegistry      tickets{ "test-shard" };
        EloService                eloService{ users };
        GameHistoryService        gameHistoryService{ nullptr }; // no Postgres in tests
        GameStateMirrorService    gameStateMirror{ nullptr, registry }; // no Redis in tests
        asio::io_context          io;
        BroadcasterManager        broadcasters{ bus, registry, [](MessageDispatcher::ConnectionHandle, const std::string&) {} };
        std::vector<std::string> pushed;
        INatsClient*              nats;
        MessageDispatcher          dispatcher;

        explicit Fixture(INatsClient* natsArg = nullptr)
            : nats(natsArg),
              dispatcher(users, clientSessions, registry, bus, tickets, broadcasters, eloService, gameHistoryService,
                         gameStateMirror,
                         [this](MessageDispatcher::ConnectionHandle, const std::string& text) { pushed.push_back(text); },
                         io, nullptr, nats) {}
    };

    nlohmann::json send(Fixture& f, MessageDispatcher::ConnectionHandle hdl, MessageType type, const nlohmann::json& payload = {}) {
        std::string reply = f.dispatcher.process(hdl, MessageCodec::encode(Message{ type, payload }));
        return nlohmann::json::parse(reply);
    }
}

TEST_CASE("MessageDispatcher - LOGIN מנותב ל-LoginHandler ומצליח") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = send(f, hdlA, MessageType::Login, { {"username", "alice"}, {"password", "pw"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("elo").get<int>() == 1200);
}

TEST_CASE("MessageDispatcher - JOIN_ROOM מנותב ל-JoinRoomHandler; חדר לא קיים נכשל") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = send(f, hdlA, MessageType::JoinRoom, { {"name", "no-such-room"} });
    CHECK_FALSE(reply.at("success").get<bool>());
}

TEST_CASE("MessageDispatcher - FIND_GAME מנותב ל-FindGameHandler ומפרסם matchmaking.request") {
    FakeNatsClient fakeNats;
    Fixture f{ &fakeNats };
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    send(f, hdlA, MessageType::Login, { {"username", "alice"}, {"password", "pw"} });

    nlohmann::json published;
    fakeNats.subscribe("matchmaking.request", [&](const nlohmann::json& payload) { published = payload; });

    nlohmann::json waiting = send(f, hdlA, MessageType::FindGame);

    CHECK(waiting.at("message").get<std::string>() == "searching for opponent");
    CHECK(waiting.at("type").get<std::string>() == "FIND_GAME"); // plain ack, tagged with the request's own type
    CHECK(published.at("username").get<std::string>() == "alice");
    CHECK(published.at("elo").get<int>() == 1200);
}

TEST_CASE("MessageDispatcher - matchmaking.assigned נדחף כ-GAME_FOUND לחיבור הנכון") {
    // Exercises the actual cross-shard bug this rewrite fixes: the pairing
    // itself (Matchmaker service) and the room allocation (GameAllocator)
    // both run out of process now - this simulates their combined answer
    // arriving over NATS, the way it would for two players matched from
    // different shards, and checks FindGameHandler's subscription (wired
    // up by MessageDispatcher's constructor) turns it into the right push.
    FakeNatsClient fakeNats;
    Fixture f{ &fakeNats };
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    send(f, hdlA, MessageType::Login, { {"username", "alice"}, {"password", "pw"} });
    send(f, hdlA, MessageType::FindGame); // mints ticket "test-shard#1" (MatchTicketRegistry's first, for this fixture)

    fakeNats.publish("matchmaking.assigned",
                      { {"ticketA", "test-shard#1"}, {"roleA", "White"},
                        {"ticketB", "other-shard#7"}, {"roleB", "Black"},
                        {"roomName", "match-1"}, {"shard", "ws://gameserver-a:9002"},
                        {"whiteName", "alice"}, {"whiteElo", 1200},
                        {"blackName", "bob"}, {"blackElo", 1250} });
    f.io.poll(); // the subscription marshals the actual push through io_context::post

    REQUIRE(f.pushed.size() == 1);
    nlohmann::json pushedMsg = nlohmann::json::parse(f.pushed[0]);
    CHECK(pushedMsg.at("type").get<std::string>() == "GAME_FOUND");
    CHECK(pushedMsg.at("payload").at("role").get<std::string>() == "White");
    CHECK(pushedMsg.at("payload").at("roomName").get<std::string>() == "match-1");
    CHECK(pushedMsg.at("payload").at("shard").get<std::string>() == "ws://gameserver-a:9002");
}

TEST_CASE("MessageDispatcher - MOVE ללא handler רשום נופל ל-CommandDispatcher ומצליח על הלוח האמיתי") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    // No CreateRoom message type anymore (regular rooms go through the API
    // Gateway/Game Allocator over NATS) - build the room directly on the
    // fixture's own registry, same as test_BroadcasterManager.cpp does.
    f.registry.createRoom("room-a", makeStartingBoard(), f.bus);
    f.registry.joinRoom("room-a", hdlA); // creator -> White, real starting board

    nlohmann::json reply = send(f, hdlA, MessageType::Move,
        { {"from", {{"row", 6}, {"col", 0}}}, {"to", {{"row", 5}, {"col", 0}}} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("role").get<std::string>() == "White");
}

TEST_CASE("MessageDispatcher - MOVE מחיבור שלא נמצא בשום חדר נכשל בלי לקרוס") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = send(f, hdlA, MessageType::Move,
        { {"from", {{"row", 6}, {"col", 0}}}, {"to", {{"row", 5}, {"col", 0}}} });

    CHECK_FALSE(reply.at("success").get<bool>());
}
