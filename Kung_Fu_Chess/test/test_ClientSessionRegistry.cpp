#include "../doctest.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include <memory>

namespace {
    // connection_hdl is a weak_ptr<void> - keep the shared_ptr it's built
    // from alive for as long as the handle needs to stay "connected" in a
    // test, exactly like a real websocketpp connection keeps it alive.
    ClientSessionRegistry::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("ClientSessionRegistry - sessionFor מחזיר nullptr לחיבור שלא התחבר") {
    ClientSessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    CHECK(registry.sessionFor(hdlA) == nullptr);
}

TEST_CASE("ClientSessionRegistry - onLogin יוצר session עם username ו-elo נכונים") {
    ClientSessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.onLogin(hdlA, "alice", 1350);

    const ClientSession* session = registry.sessionFor(hdlA);
    REQUIRE(session != nullptr);
    CHECK(session->username == "alice");
    CHECK(session->elo == 1350);
    CHECK_FALSE(session->searchingForGame);
}

TEST_CASE("ClientSessionRegistry - onDisconnect מוחק את ה-session") {
    ClientSessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.onLogin(hdlA, "alice", 1350);

    registry.onDisconnect(hdlA);

    CHECK(registry.sessionFor(hdlA) == nullptr);
}

TEST_CASE("ClientSessionRegistry - onDisconnect על חיבור שלא התחבר לא קורס") {
    ClientSessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.onDisconnect(hdlA);
    CHECK(registry.sessionFor(hdlA) == nullptr);
}

TEST_CASE("ClientSessionRegistry - חיבורים שונים מנוהלים בנפרד") {
    ClientSessionRegistry registry;
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.onLogin(hdlA, "alice", 1200);
    registry.onLogin(hdlB, "bob", 1400);

    CHECK(registry.sessionFor(hdlA)->username == "alice");
    CHECK(registry.sessionFor(hdlA)->elo == 1200);
    CHECK(registry.sessionFor(hdlB)->username == "bob");
    CHECK(registry.sessionFor(hdlB)->elo == 1400);
}

TEST_CASE("ClientSessionRegistry - onLogin חוזר על אותו חיבור מעדכן את ה-session הקיים") {
    ClientSessionRegistry registry;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.onLogin(hdlA, "alice", 1200);
    registry.onLogin(hdlA, "alice", 1250);

    CHECK(registry.sessionFor(hdlA)->elo == 1250);
}
