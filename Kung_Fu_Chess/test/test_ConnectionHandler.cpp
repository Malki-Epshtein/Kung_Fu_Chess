#include "../doctest.h"
#include "../server/app/session/ConnectionHandler.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../server/app/session/GameSession.h"
#include "../server/app/logic/MatchTicketRegistry.h"
#include "../shared/model/Board.h"
#include <asio/io_context.hpp>
#include <chrono>
#include <memory>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    ConnectionHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("ConnectionHandler - onClose על חיבור שלא היה בחדר מנקה session וטיקט matchmaking בלי לקרוס") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    MatchTicketRegistry tickets{ "shard" };
    asio::io_context io;
    ConnectionHandler handler(registry, clientSessions, tickets, nullptr, io);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    clientSessions.onLogin(hdlA, "alice", 1200);
    tickets.add(hdlA);

    handler.onClose(hdlA);

    CHECK(clientSessions.sessionFor(hdlA) == nullptr);
    CHECK_FALSE(tickets.cancel(hdlA).has_value()); // already gone - onClose already cancelled it
}

TEST_CASE("ConnectionHandler - onClose מוציא את השחקן מהחדר שלו") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    MatchTicketRegistry tickets{ "shard" };
    EventBus bus;
    asio::io_context io;
    ConnectionHandler handler(registry, clientSessions, tickets, nullptr, io);

    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.joinRoom("room-a", hdlA);

    handler.onClose(hdlA);

    CHECK(registry.roomOf(hdlA) == nullptr);
}

TEST_CASE("ConnectionHandler - onClose של שחקן מושב (White) מפעיל disconnect countdown") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    MatchTicketRegistry tickets{ "shard" };
    EventBus bus;
    asio::io_context io;
    ConnectionHandler handler(registry, clientSessions, tickets, nullptr, io);

    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA); // first joiner -> White
    registry.joinRoom("room-a", hdlB); // second joiner -> Black - stays seated, so the room isn't emptied (and freed) by White's disconnect below

    handler.onClose(hdlA);
    io.run_for(std::chrono::milliseconds(50)); // let the immediately-scheduled countdown tick run once

    // computeStep() builds the same payload tick() sends, without needing a
    // snapshotSender_/EventBus subscriber wired up - disconnect status rides
    // on it directly (see GameSession::computeStep).
    nlohmann::json received = registry.room("room-a")->computeStep(30);

    REQUIRE(received.contains("disconnect"));
    CHECK(received.at("disconnect").at("active") == true);
    CHECK(received.at("disconnect").at("color") == "White");
}

TEST_CASE("ConnectionHandler - onClose של צופה לא מפעיל disconnect countdown") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    MatchTicketRegistry tickets{ "shard" };
    EventBus bus;
    asio::io_context io;
    ConnectionHandler handler(registry, clientSessions, tickets, nullptr, io);

    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a, b, c;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    auto hdlC = handleFrom(c);
    registry.joinRoom("room-a", hdlA); // White
    registry.joinRoom("room-a", hdlB); // Black
    registry.joinRoom("room-a", hdlC); // Spectator

    handler.onClose(hdlC);
    io.poll();

    nlohmann::json received;
    bus.subscribe(GameSession::snapshotTopic("room-a"), [&](const nlohmann::json& data) { received = data; });
    registry.room("room-a")->tick(30);

    CHECK_FALSE(received.contains("disconnect"));
}
