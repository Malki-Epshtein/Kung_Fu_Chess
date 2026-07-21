#include "../doctest.h"
#include "../server/app/session/GameSession.h"
#include "../shared/model/Board.h"
#include "../shared/protocol/MoveLogCodec.h"
#include <memory>

namespace {
    class TestPiece : public Piece {
    public:
        TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
            : Piece(id, color, kind, cell, Chess::State::Idle) {}
        std::vector<Position> getValidMoves() const override { return {}; }
    };

    std::shared_ptr<Piece> make(int id, Chess::Color color, Chess::Kind kind, Position pos) {
        return std::make_shared<TestPiece>(id, color, kind, pos);
    }
}

TEST_CASE("GameSession - tick מפרסם snapshot לנושא הנכון על ה-bus") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received;
    bool called = false;
    bus.subscribe(GameSession::snapshotTopic("room-a"), [&](const nlohmann::json& data) {
        called = true;
        received = data;
    });

    session.tick(30);

    CHECK(called);
    CHECK(received.at("board_width") == 8);
    CHECK(received.at("board_height") == 8);
}

TEST_CASE("GameSession - חושף את ה-engine שלה לשימוש חיצוני (למשל CommandDispatcher)") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    CHECK_FALSE(session.engine().isGameOver());
}

// ==================== Stage D: disconnect status ====================

TEST_CASE("GameSession - ללא ניתוק פעיל, השידור לא מכיל מפתח disconnect") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received;
    bus.subscribe(GameSession::snapshotTopic("room-a"), [&](const nlohmann::json& data) { received = data; });

    session.tick(30);

    CHECK_FALSE(received.contains("disconnect"));
}

TEST_CASE("GameSession - ניתוק פעיל מתווסף לשידור הרגיל") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");
    session.setDisconnectStatus({ true, Chess::Color::White, 17 });

    nlohmann::json received;
    bus.subscribe(GameSession::snapshotTopic("room-a"), [&](const nlohmann::json& data) { received = data; });

    session.tick(30);

    REQUIRE(received.contains("disconnect"));
    CHECK(received.at("disconnect").at("active") == true);
    CHECK(received.at("disconnect").at("color") == "White");
    CHECK(received.at("disconnect").at("secondsRemaining") == 17);
}

// ==================== Score + move log (Stage I) ====================

TEST_CASE("GameSession - score מתחיל 0-0 בשידור") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received;
    bus.subscribe(GameSession::snapshotTopic("room-a"), [&](const nlohmann::json& data) { received = data; });

    session.tick(30);

    REQUIRE(received.contains("score"));
    CHECK(received.at("score").at("white") == 0);
    CHECK(received.at("score").at("black") == 0);
}

TEST_CASE("GameSession - אכילה מעדכנת את הניקוד בשידור הבא") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {3, 4}), {3, 4});
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received;
    bus.subscribe(GameSession::snapshotTopic("room-a"), [&](const nlohmann::json& data) { received = data; });

    MoveResult result = session.engine().requestMove({3, 3}, {3, 4});
    REQUIRE(result.is_accepted);
    session.tick(1000); // one square -> 1000ms travel time, arrives and resolves this same tick

    CHECK(received.at("score").at("white") == 1); // captured a Pawn (value 1)
    CHECK(received.at("score").at("black") == 0);
}

TEST_CASE("GameSession - מהלך שהושלם מפורסם ל-moveLogTopic מיד") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received;
    bool called = false;
    bus.subscribe(GameSession::moveLogTopic("room-a"), [&](const nlohmann::json& data) {
        called = true;
        received = data;
    });

    MoveResult result = session.engine().requestMove({3, 3}, {3, 6});
    REQUIRE(result.is_accepted);
    session.tick(3000); // three squares -> 3000ms travel time

    REQUIRE(called);
    CHECK(received.at("type") == "MOVE_LOGGED");
    CHECK(received.at("payload").at("color") == "White");
    CHECK(received.at("payload").at("notation") == "Rg5");
}

TEST_CASE("GameSession - fullMoveLog משקף את המהלכים שכבר הושלמו") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    EventBus bus;
    GameSession session(board, bus, "room-a");

    CHECK(MoveLogCodec::decodeAll(session.fullMoveLog()).white.empty());

    MoveResult result = session.engine().requestMove({3, 3}, {3, 6});
    REQUIRE(result.is_accepted);
    session.tick(3000);

    MoveLogBundle bundle = MoveLogCodec::decodeAll(session.fullMoveLog());
    REQUIRE(bundle.white.size() == 1);
    CHECK(bundle.white[0].notation == "Rg5");
    CHECK(bundle.black.empty());
}
