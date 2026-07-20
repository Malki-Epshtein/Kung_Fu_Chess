#include "../doctest.h"
#include "../server/app/SessionRegistry.h"
#include "../shared/model/Board.h"
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
