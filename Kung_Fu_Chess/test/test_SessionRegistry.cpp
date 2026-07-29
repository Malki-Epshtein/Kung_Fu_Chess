#include "../doctest.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/GameSession.h"
#include "../shared/model/Board.h"
#include <asio/io_context.hpp>
#include <chrono>
#include <memory>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    // connection_hdl is a weak_ptr<void> - keep the shared_ptr it's built
    // from alive for as long as the handle needs to stay "connected" in a
    // test, exactly like a real websocketpp connection keeps it alive.
    SessionRegistry::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("SessionRegistry - יצירת חדר עם שם פנוי מצליחה") {
    SessionRegistry registry;
    EventBus bus;
    CHECK(registry.createRoom("room-a", makeBoard(), bus));
    CHECK(registry.roomExists("room-a"));
}

TEST_CASE("SessionRegistry - יצירת חדר עם שם תפוס נכשלת") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    CHECK_FALSE(registry.createRoom("room-a", makeBoard(), bus));
}

TEST_CASE("SessionRegistry - room מחזיר nullptr לחדר שלא קיים") {
    SessionRegistry registry;
    CHECK(registry.room("no-such-room") == nullptr);
}

TEST_CASE("SessionRegistry - הצטרפות לפי סדר מקצה לבן, שחור, צופה") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b, c;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    auto hdlC = handleFrom(c);

    CHECK(registry.joinRoom("room-a", hdlA));
    CHECK(registry.joinRoom("room-a", hdlB));
    CHECK(registry.joinRoom("room-a", hdlC));

    CHECK(registry.roleOf(hdlA) == Chess::Color::White);
    CHECK(registry.roleOf(hdlB) == Chess::Color::Black);
    CHECK(registry.roleOf(hdlC) == Chess::Color::None);
}

TEST_CASE("SessionRegistry - הצטרפות לחדר שלא קיים נכשלת") {
    SessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    CHECK_FALSE(registry.joinRoom("no-such-room", hdlA));
}

TEST_CASE("SessionRegistry - roomOf מחזיר את שם החדר הנכון") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.joinRoom("room-a", hdlA);

    const std::string* name = registry.roomOf(hdlA);
    REQUIRE(name != nullptr);
    CHECK(*name == "room-a");
}

TEST_CASE("SessionRegistry - roomOf מחזיר nullptr לחיבור שלא הצטרף לשום חדר") {
    SessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    CHECK(registry.roomOf(hdlA) == nullptr);
}

TEST_CASE("SessionRegistry - leave מסיר את החיבור ומחזיר את התפקיד הקודם שלו") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.joinRoom("room-a", hdlA);

    CHECK(registry.leave(hdlA) == Chess::Color::White);
    CHECK(registry.roomOf(hdlA) == nullptr);
    CHECK(registry.roleOf(hdlA) == Chess::Color::None);
}

TEST_CASE("SessionRegistry - leave על חיבור לא ידוע לא קורס ומחזיר None") {
    SessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    CHECK(registry.leave(hdlA) == Chess::Color::None);
}

TEST_CASE("SessionRegistry - roomNames מחזיר את כל החדרים הקיימים") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    registry.createRoom("room-b", makeBoard(), bus);

    auto names = registry.roomNames();
    CHECK(names.size() == 2);
}

TEST_CASE("SessionRegistry - connectionsInRoom מחזיר את כל החיבורים של החדר") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-a", hdlB);

    auto connections = registry.connectionsInRoom("room-a");
    CHECK(connections.size() == 2);
}

TEST_CASE("SessionRegistry - connectionsInRoom לא כולל חיבורים של חדר אחר") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    registry.createRoom("room-b", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-b", hdlB);

    auto connections = registry.connectionsInRoom("room-a");
    CHECK(connections.size() == 1);
}

TEST_CASE("SessionRegistry - connectionsInRoom מחזיר רשימה ריקה לחדר שלא קיים") {
    SessionRegistry registry;
    CHECK(registry.connectionsInRoom("no-such-room").empty());
}

TEST_CASE("SessionRegistry - חדרים שונים מנהלים תפקידים בנפרד") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    registry.createRoom("room-b", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);

    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-b", hdlB);

    // Both are the first joiner of their own room, so both are White there.
    CHECK(registry.roleOf(hdlA) == Chess::Color::White);
    CHECK(registry.roleOf(hdlB) == Chess::Color::White);
}

TEST_CASE("SessionRegistry - startDisconnectCountdown שומר את המקום, מצטרפים מחדש עם אותו username מקבלים את הצבע הקודם") {
    SessionRegistry registry;
    EventBus bus;
    asio::io_context io;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b, c;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA, "alice"); // White
    registry.joinRoom("room-a", hdlB, "bob");   // Black - keeps the room from being freed

    registry.leave(hdlA);
    registry.startDisconnectCountdown("room-a", Chess::Color::White, "alice", 1200, io);

    // No reserveSeats() was ever called for this room (it wasn't PLAY-matched) -
    // without the seat being held, this third joinRoom would fall through to
    // connectionCount-based arrival order and land as Spectator.
    auto hdlA2 = handleFrom(c);
    CHECK(registry.joinRoom("room-a", hdlA2, "alice"));
    CHECK(registry.roleOf(hdlA2) == Chess::Color::White);
}

TEST_CASE("SessionRegistry - cancelDisconnectCountdown מנקה את ה-disconnect banner") {
    SessionRegistry registry;
    EventBus bus;
    asio::io_context io;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA, "alice");
    registry.joinRoom("room-a", hdlB, "bob");

    registry.leave(hdlA);
    registry.startDisconnectCountdown("room-a", Chess::Color::White, "alice", 1200, io);
    io.run_for(std::chrono::milliseconds(50)); // let the immediately-scheduled tick run once

    CHECK(registry.cancelDisconnectCountdown("room-a", Chess::Color::White, "alice"));

    nlohmann::json received = registry.room("room-a")->computeStep(30);
    CHECK_FALSE(received.contains("disconnect"));
}

TEST_CASE("SessionRegistry - אחרי cancelDisconnectCountdown הטיימר לא ממשיך וה-auto-resign לא קורה") {
    SessionRegistry registry;
    EventBus bus;
    asio::io_context io;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA, "alice");
    registry.joinRoom("room-a", hdlB, "bob");

    bool ended = false;
    bus.subscribe(GameSession::gameEndedTopic("room-a"), [&](const nlohmann::json&) { ended = true; });

    registry.leave(hdlA);
    registry.startDisconnectCountdown("room-a", Chess::Color::White, "alice", 1200, io);
    io.run_for(std::chrono::milliseconds(50));

    CHECK(registry.cancelDisconnectCountdown("room-a", Chess::Color::White, "alice"));
    io.run_for(std::chrono::milliseconds(50)); // let the now-aborted wait complete (it just returns)
    CHECK_FALSE(ended);

    // Nothing is scheduled on the timer anymore, so re-running the io_context
    // for "longer than the grace period" completes near-instantly with no
    // further work - proving the countdown really stopped, not just that we
    // haven't waited long enough yet.
    io.restart();
    io.run_for(std::chrono::seconds(1));
    CHECK_FALSE(ended);
}

TEST_CASE("SessionRegistry - cancelDisconnectCountdown עם username אחר נכשל והטיימר ממשיך") {
    SessionRegistry registry;
    EventBus bus;
    asio::io_context io;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA, "alice");
    registry.joinRoom("room-a", hdlB, "bob");

    registry.leave(hdlA);
    registry.startDisconnectCountdown("room-a", Chess::Color::White, "alice", 1200, io);
    io.run_for(std::chrono::milliseconds(50));

    CHECK_FALSE(registry.cancelDisconnectCountdown("room-a", Chess::Color::White, "mallory"));

    nlohmann::json received = registry.room("room-a")->computeStep(30);
    REQUIRE(received.contains("disconnect"));
    CHECK(received.at("disconnect").at("active") == true);
}
