#include "../doctest.h"
#include "../server/app/handlers/JoinRoomHandler.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../shared/model/Board.h"
#include "../shared/protocol/MoveLogCodec.h"
#include <memory>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    JoinRoomHandler::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }

    class TestPiece : public Piece {
    public:
        TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
            : Piece(id, color, kind, cell, Chess::State::Idle) {}
        std::vector<Position> getValidMoves() const override { return {}; }
    };
}

TEST_CASE("JoinRoomHandler - הצטרפות לחדר קיים מצליחה ונותנת Black לשני") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    JoinRoomHandler handler(registry, clientSessions);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    registry.joinRoom("room-a", hdlA); // first occupant, White

    nlohmann::json reply = handler.handle(hdlB, { {"name", "room-a"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("role").get<std::string>() == "Black");
    CHECK(reply.at("roomName").get<std::string>() == "room-a");
    CHECK(registry.roleOf(hdlB) == Chess::Color::Black);

    REQUIRE(reply.contains("moveLog"));
    MoveLogBundle bundle = MoveLogCodec::decodeAll(reply.at("moveLog"));
    CHECK(bundle.white.empty());
    CHECK(bundle.black.empty());
}

TEST_CASE("JoinRoomHandler - הצטרפות לחדר שלא קיים נכשלת") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    JoinRoomHandler handler(registry, clientSessions);

    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    nlohmann::json reply = handler.handle(hdlA, { {"name", "no-such-room"} });

    CHECK_FALSE(reply.at("success").get<bool>());
    CHECK(registry.roomOf(hdlA) == nullptr);
}

TEST_CASE("JoinRoomHandler - התשובה כוללת שם/elo של White ושל המצטרף עצמו כ-Black") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    JoinRoomHandler handler(registry, clientSessions);

    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    clientSessions.onLogin(hdlA, "alice", 1250);
    clientSessions.onLogin(hdlB, "bob", 1400);
    registry.joinRoom("room-a", hdlA); // White

    nlohmann::json reply = handler.handle(hdlB, { {"name", "room-a"} });

    CHECK(reply.at("whiteName").get<std::string>() == "alice");
    CHECK(reply.at("whiteElo").get<int>() == 1250);
    CHECK(reply.at("blackName").get<std::string>() == "bob");
    CHECK(reply.at("blackElo").get<int>() == 1400);
}

TEST_CASE("JoinRoomHandler - צופה שמצטרף באמצע משחק מקבל את היומן שכבר נצבר (backfill)") {
    SessionRegistry registry;
    ClientSessionRegistry clientSessions;
    EventBus bus;
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(std::make_shared<TestPiece>(1, Chess::Color::White, Chess::Kind::Rook, Position{3, 3}), {3, 3});
    registry.createRoom("room-a", board, bus);
    JoinRoomHandler handler(registry, clientSessions);

    // A move already completed in this room before anyone new joins - the
    // exact scenario a late-joining spectator needs backfilled.
    MoveResult result = registry.room("room-a")->engine().requestMove({3, 3}, {3, 6});
    REQUIRE(result.is_accepted);
    registry.room("room-a")->tick(3000); // three squares -> 3000ms travel time

    std::shared_ptr<int> spectator;
    auto hdlSpectator = handleFrom(spectator);
    nlohmann::json reply = handler.handle(hdlSpectator, { {"name", "room-a"} });

    REQUIRE(reply.contains("moveLog"));
    MoveLogBundle bundle = MoveLogCodec::decodeAll(reply.at("moveLog"));
    REQUIRE(bundle.white.size() == 1);
    CHECK(bundle.white[0].notation == "Rg5");
    CHECK(bundle.black.empty());
}
