#include "../doctest.h"
#include "../server/app/session/RoomIdentityResolver.h"
#include "../shared/model/Board.h"
#include "../shared/bus/EventBus.h"
#include <memory>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    SessionRegistry::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("RoomIdentityResolver - חדר עם רק White מחזיר שם/elo של White ושחור ריק") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    clientSessions.onLogin(hdlA, "alice", 1250);
    registry.joinRoom("room-a", hdlA);

    RoomIdentity identity = RoomIdentityResolver::resolve(registry, clientSessions, "room-a");

    CHECK(identity.whiteUsername == "alice");
    CHECK(identity.whiteElo == 1250);
    CHECK(identity.blackUsername == "");
    CHECK(identity.blackElo == 0);
    CHECK(identity.spectatorCount == 0);
}

TEST_CASE("RoomIdentityResolver - White ו-Black שניהם מזוהים נכון") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    clientSessions.onLogin(hdlA, "alice", 1250);
    clientSessions.onLogin(hdlB, "bob", 1400);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-a", hdlB);

    RoomIdentity identity = RoomIdentityResolver::resolve(registry, clientSessions, "room-a");

    CHECK(identity.whiteUsername == "alice");
    CHECK(identity.whiteElo == 1250);
    CHECK(identity.blackUsername == "bob");
    CHECK(identity.blackElo == 1400);
}

TEST_CASE("RoomIdentityResolver - צופים נספרים אבל לא נכנסים לרשימת שמות") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a, b, c;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    auto hdlC = handleFrom(c);
    clientSessions.onLogin(hdlA, "alice", 1200);
    clientSessions.onLogin(hdlB, "bob", 1200);
    clientSessions.onLogin(hdlC, "carol", 1200);
    registry.joinRoom("room-a", hdlA);   // White
    registry.joinRoom("room-a", hdlB);   // Black
    registry.joinRoom("room-a", hdlC);   // Spectator

    RoomIdentity identity = RoomIdentityResolver::resolve(registry, clientSessions, "room-a");

    CHECK(identity.spectatorCount == 1);
}

TEST_CASE("RoomIdentityResolver - חדר שלא קיים מחזיר identity ריק לגמרי") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;

    RoomIdentity identity = RoomIdentityResolver::resolve(registry, clientSessions, "no-such-room");

    CHECK(identity.whiteUsername == "");
    CHECK(identity.blackUsername == "");
    CHECK(identity.spectatorCount == 0);
}

TEST_CASE("RoomIdentityResolver - חיבור בתפקיד White/Black שלא התחבר (אין ClientSession) לא קורס") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    // Joined the room but never logged in - sessionFor(hdlA) is nullptr.
    registry.joinRoom("room-a", hdlA);

    RoomIdentity identity = RoomIdentityResolver::resolve(registry, clientSessions, "room-a");

    CHECK(identity.whiteUsername == "");
    CHECK(identity.whiteElo == 0);
}
