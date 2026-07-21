#include "../doctest.h"
#include "../server/app/handlers/CreateRoomHandler.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../shared/model/Board.h"
#include "../shared/protocol/MoveLogCodec.h"
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
    ClientSessionRegistry clientSessions;
    EventBus bus;
    std::vector<std::string> attachedRooms;
    CreateRoomHandler handler(registry, clientSessions, bus, [&](const std::string& name) { attachedRooms.push_back(name); });

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    nlohmann::json reply = handler.handle(hdlA, { {"name", "room-a"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("role").get<std::string>() == "White");
    CHECK(reply.at("roomName").get<std::string>() == "room-a");
    CHECK(registry.roomExists("room-a"));
    CHECK(registry.roleOf(hdlA) == Chess::Color::White);

    REQUIRE(reply.contains("moveLog"));
    MoveLogBundle bundle = MoveLogCodec::decodeAll(reply.at("moveLog"));
    CHECK(bundle.white.empty());
    CHECK(bundle.black.empty());
}

TEST_CASE("CreateRoomHandler - יצירת חדר מצרפת broadcaster לחדר החדש") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    std::vector<std::string> attachedRooms;
    CreateRoomHandler handler(registry, clientSessions, bus, [&](const std::string& name) { attachedRooms.push_back(name); });

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    handler.handle(hdlA, { {"name", "room-a"} });

    REQUIRE(attachedRooms.size() == 1);
    CHECK(attachedRooms[0] == "room-a");
}

TEST_CASE("CreateRoomHandler - שם תפוס נכשל ולא נוגע בחדר הקיים") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    CreateRoomHandler handler(registry, clientSessions, bus, [](const std::string&) {});

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    handler.handle(hdlA, { {"name", "room-a"} });

    nlohmann::json reply = handler.handle(hdlB, { {"name", "room-a"} });

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(registry.roleOf(hdlB) == Chess::Color::None);
}

TEST_CASE("CreateRoomHandler - התשובה כוללת שם/elo נכונים של היוצר ותאים ריקים ל-Black") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    CreateRoomHandler handler(registry, clientSessions, bus, [](const std::string&) {});

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    clientSessions.onLogin(hdlA, "alice", 1250);
    nlohmann::json reply = handler.handle(hdlA, { {"name", "room-a"} });

    CHECK(reply.at("whiteName").get<std::string>() == "alice");
    CHECK(reply.at("whiteElo").get<int>() == 1250);
    CHECK(reply.at("blackName").get<std::string>() == "");
    CHECK(reply.at("blackElo").get<int>() == 0);
}
