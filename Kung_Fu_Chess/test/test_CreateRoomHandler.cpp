#include "../doctest.h"
#include "../server/app/handlers/CreateRoomHandler.h"
#include "../server/app/session/SessionRegistry.h"
#include "../shared/model/Board.h"
#include <memory>
#include <vector>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    CreateRoomHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("CreateRoomHandler - שם פנוי נוצר בהצלחה והיוצר הופך ל-White") {
    SessionRegistry registry;
    EventBus bus;
    std::vector<std::string> attachedRooms;
    CreateRoomHandler handler(registry, bus, [&](const std::string& name) { attachedRooms.push_back(name); });

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    nlohmann::json reply = handler.handle(hdlA, { {"name", "room-a"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("role").get<std::string>() == "White");
    CHECK(reply.at("roomName").get<std::string>() == "room-a");
    CHECK(registry.roomExists("room-a"));
    CHECK(registry.roleOf(hdlA) == Chess::Color::White);
}

TEST_CASE("CreateRoomHandler - יצירת חדר מצרפת broadcaster לחדר החדש") {
    SessionRegistry registry;
    EventBus bus;
    std::vector<std::string> attachedRooms;
    CreateRoomHandler handler(registry, bus, [&](const std::string& name) { attachedRooms.push_back(name); });

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    handler.handle(hdlA, { {"name", "room-a"} });

    REQUIRE(attachedRooms.size() == 1);
    CHECK(attachedRooms[0] == "room-a");
}

TEST_CASE("CreateRoomHandler - שם תפוס נכשל ולא נוגע בחדר הקיים") {
    SessionRegistry registry;
    EventBus bus;
    CreateRoomHandler handler(registry, bus, [](const std::string&) {});

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    handler.handle(hdlA, { {"name", "room-a"} });

    nlohmann::json reply = handler.handle(hdlB, { {"name", "room-a"} });

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(registry.roleOf(hdlB) == Chess::Color::None);
}
