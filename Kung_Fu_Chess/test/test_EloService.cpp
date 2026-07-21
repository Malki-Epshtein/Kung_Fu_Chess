#include "../doctest.h"
#include "../server/app/logic/EloService.h"
#include "../server/app/session/GameSession.h"
#include "../server/app/session/GameResultCodec.h"
#include "../server/db/UserRepository.h"

// Proves the full gameEndedTopic -> EloService -> UserRepository wiring
// with a real EventBus and a real (in-memory) UserRepository - no
// WsServer, no sockets, no asio timers. This is exactly what couldn't be
// unit-tested before EloService existed: the ELO application used to live
// inline in WsServer.cpp's tick loop, only provable by an actual running
// server and a real 20s wait for the disconnect grace period.

TEST_CASE("EloService - מגיב לגמר משחק ב-gameEndedTopic ומעדכן את שני השחקנים") {
    UserRepository users(":memory:");
    users.login("alice", "pw"); // starts at 1200
    users.login("bob", "pw");   // starts at 1200

    EventBus bus;
    EloService eloService(users);
    eloService.attach(bus, "room-a");

    GameResult result{ Chess::Color::White, GameEndReason::KingCapture, "alice", 1200, "bob", 1200 };
    bus.publish(GameSession::gameEndedTopic("room-a"), GameResultCodec::encode(result));

    CHECK(users.login("alice", "pw").elo == 1216);
    CHECK(users.login("bob", "pw").elo == 1184);
}

TEST_CASE("EloService - לא נוגע ב-ELO אם צד אחד ריק (מושב לא מולא)") {
    UserRepository users(":memory:");
    users.login("alice", "pw");

    EventBus bus;
    EloService eloService(users);
    eloService.attach(bus, "room-a");

    GameResult result{ Chess::Color::White, GameEndReason::KingCapture, "alice", 1200, "", 0 };
    bus.publish(GameSession::gameEndedTopic("room-a"), GameResultCodec::encode(result));

    CHECK(users.login("alice", "pw").elo == 1200); // unchanged
}

TEST_CASE("EloService - חדרים שונים לא משפיעים אחד על השני") {
    UserRepository users(":memory:");
    users.login("alice", "pw");
    users.login("bob", "pw");
    users.login("carol", "pw");

    EventBus bus;
    EloService eloService(users);
    eloService.attach(bus, "room-a");
    eloService.attach(bus, "room-b");

    // Only room-a's topic fires - carol (room-b's would-be participant)
    // must be untouched.
    GameResult result{ Chess::Color::White, GameEndReason::KingCapture, "alice", 1200, "bob", 1200 };
    bus.publish(GameSession::gameEndedTopic("room-a"), GameResultCodec::encode(result));

    CHECK(users.login("alice", "pw").elo == 1216);
    CHECK(users.login("carol", "pw").elo == 1200); // unchanged
}
