#include "../doctest.h"
#include "../server/app/handlers/JoinRoomHandler.h"
#include "../server/app/session/SessionRegistry.h"
#include "../shared/model/Board.h"
#include <memory>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    JoinRoomHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("JoinRoomHandler - הצטרפות לחדר קיים מצליחה ונותנת Black לשני") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    JoinRoomHandler handler(registry);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA); // first occupant, White

    nlohmann::json reply = handler.handle(hdlB, { {"name", "room-a"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("role").get<std::string>() == "Black");
    CHECK(reply.at("roomName").get<std::string>() == "room-a");
    CHECK(registry.roleOf(hdlB) == Chess::Color::Black);
}

TEST_CASE("JoinRoomHandler - הצטרפות לחדר שלא קיים נכשלת") {
    SessionRegistry registry;
    JoinRoomHandler handler(registry);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    nlohmann::json reply = handler.handle(hdlA, { {"name", "no-such-room"} });

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(registry.roomOf(hdlA) == nullptr);
}
