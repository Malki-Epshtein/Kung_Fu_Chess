#include "../doctest.h"
#include "../server/app/networking/BroadcasterManager.h"
#include "../server/app/networking/NetworkBroadcaster.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/GameSession.h"
#include "../shared/model/Board.h"
#include <memory>
#include <vector>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    BroadcasterManager::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("BroadcasterManager - attach שולח snapshot רק לחיבורים של אותו חדר") {
    SessionRegistry registry;
    EventBus bus;

    std::vector<std::string> sentTo;
    BroadcasterManager manager(bus, registry, [&](BroadcasterManager::ConnectionHandle, const std::string& text) {
        sentTo.push_back(text);
    });

    // Created AFTER the manager exists (like AllocateRoomHandler does:
    // create the room, then attach exactly once) - attaching a room the
    // constructor already auto-attached would leave a dangling EventBus
    // subscriber pointing at the overwritten NetworkBroadcaster.
    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-a", hdlB);

    manager.attach("room-a");
    registry.room("room-a")->tick(30); // snapshot delivery goes through tick()'s direct callback now, not EventBus

    CHECK(sentTo.size() == 2); // both occupants of room-a
}

TEST_CASE("BroadcasterManager - attach שולח גם אירועי moveLogTopic לחיבורים של אותו חדר") {
    SessionRegistry registry;
    EventBus bus;

    std::vector<std::string> sentTo;
    BroadcasterManager manager(bus, registry, [&](BroadcasterManager::ConnectionHandle, const std::string& text) {
        sentTo.push_back(text);
    });

    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-a", hdlB);

    manager.attach("room-a");
    bus.publish(GameSession::moveLogTopic("room-a"), { {"type", "MOVE_LOGGED"}, {"payload", {{"color", "White"}, {"notation", "e4"}, {"timestamp", "00:00.000"}}} });

    CHECK(sentTo.size() == 2); // both occupants of room-a
}

TEST_CASE("BroadcasterManager - attach שולח גם אירועי captureTopic לחיבורים של אותו חדר") {
    SessionRegistry registry;
    EventBus bus;

    std::vector<std::string> sentTo;
    BroadcasterManager manager(bus, registry, [&](BroadcasterManager::ConnectionHandle, const std::string& text) {
        sentTo.push_back(text);
    });

    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-a", hdlB);

    manager.attach("room-a");
    bus.publish(GameSession::captureTopic("room-a"), { {"type", "CAPTURE_EVENT"}, {"payload", {{"kind", "Pawn"}, {"color", "White"}, {"cell", {{"row", 0}, {"col", 0}}}}} });

    CHECK(sentTo.size() == 2); // both occupants of room-a
}

TEST_CASE("BroadcasterManager - חדרים שונים לא מקבלים שידורים אחד של השני") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    registry.createRoom("room-b", makeBoard(), bus);

    std::vector<BroadcasterManager::ConnectionHandle> recipients;
    BroadcasterManager manager(bus, registry, [&](BroadcasterManager::ConnectionHandle hdl, const std::string&) {
        recipients.push_back(hdl);
    });

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA);
    registry.joinRoom("room-b", hdlB);

    // manager's constructor already attached both rooms (they existed before
    // it was constructed) - no explicit attach() needed here.
    registry.room("room-a")->tick(30); // snapshot delivery goes through tick()'s direct callback now, not EventBus

    REQUIRE(recipients.size() == 1);
    CHECK(recipients[0].lock() == std::static_pointer_cast<void>(a)); // only room-a's occupant, not room-b's
}

TEST_CASE("BroadcasterManager - חדרים קיימים מראש (בזמן ה-constructor) מקבלים broadcaster אוטומטית") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    registry.joinRoom("room-a", hdlA);

    int sendCount = 0;
    // No explicit attach() call - the constructor itself should have
    // attached "room-a" since it already existed in the registry.
    BroadcasterManager manager(bus, registry, [&](BroadcasterManager::ConnectionHandle, const std::string&) {
        sendCount++;
    });

    registry.room("room-a")->tick(30); // snapshot delivery goes through tick()'s direct callback now, not EventBus

    CHECK(sendCount == 1);
}

TEST_CASE("BroadcasterManager - חדר שנוצר אחרי ה-constructor לא שולח עד שקוראים ל-attach") {
    SessionRegistry registry;
    EventBus bus;
    int managerSendCount = 0;
    BroadcasterManager manager(bus, registry, [&](BroadcasterManager::ConnectionHandle, const std::string&) {
        managerSendCount++;
    });

    // Created after the manager already exists, so it's NOT among the
    // rooms the constructor auto-attached - this is exactly the situation
    // AllocateRoomHandler handles by calling attach() itself.
    registry.createRoom("room-c", makeBoard(), bus);
    std::shared_ptr<int> c;
    auto hdlC = handleFrom(c);
    registry.joinRoom("room-c", hdlC);

    // Independent probe proves the topic itself is publishable/working.
    int probeSendCount = 0;
    NetworkBroadcaster probe(bus, GameSession::snapshotTopic("room-c"), [&](const std::string&) { probeSendCount++; });
    bus.publish(GameSession::snapshotTopic("room-c"), { {"board_width", 8} });

    CHECK(probeSendCount == 1);
    CHECK(managerSendCount == 0); // manager never attached to room-c
}
