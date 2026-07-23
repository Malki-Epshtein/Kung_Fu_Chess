#include "../doctest.h"
#include "../server/app/session/GameSession.h"
#include "../server/app/session/GameResultCodec.h"
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

TEST_CASE("GameSession - tick מחזיר snapshot עם שדות הלוח הנכונים") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received = session.computeStep(30);

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

    nlohmann::json received = session.computeStep(30);

    CHECK_FALSE(received.contains("disconnect"));
}

TEST_CASE("GameSession - ניתוק פעיל מתווסף לשידור הרגיל") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");
    session.setDisconnectStatus({ true, Chess::Color::White, 17 });

    nlohmann::json received = session.computeStep(30);

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

    nlohmann::json received = session.computeStep(30);

    REQUIRE(received.contains("score"));
    CHECK(received.at("score").at("white") == 0);
    CHECK(received.at("score").at("black") == 0);
}

TEST_CASE("GameSession - identity ריק כברירת מחדל, ומתעדכן אחרי setIdentity") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    nlohmann::json received = session.computeStep(30);
    REQUIRE(received.contains("identity"));
    CHECK(received.at("identity").at("whiteName") == "");
    CHECK(received.at("identity").at("spectatorCount") == 0);

    session.setIdentity(RoomIdentity{ "alice", 1250, "bob", 1400, 2 });
    received = session.computeStep(30);
    CHECK(received.at("identity").at("whiteName") == "alice");
    CHECK(received.at("identity").at("whiteElo") == 1250);
    CHECK(received.at("identity").at("blackName") == "bob");
    CHECK(received.at("identity").at("blackElo") == 1400);
    CHECK(received.at("identity").at("spectatorCount") == 2);
}

TEST_CASE("GameSession - אכילה מעדכנת את הניקוד בשידור הבא") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {3, 4}), {3, 4});
    EventBus bus;
    GameSession session(board, bus, "room-a");

    MoveResult result = session.engine().requestMove({3, 3}, {3, 4});
    REQUIRE(result.is_accepted);
    nlohmann::json received = session.computeStep(1000); // one square -> 1000ms travel time, arrives and resolves this same tick

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

// ==================== Game result / ELO (Stage I, step 5) ====================

TEST_CASE("GameSession - שום פרסום ל-gameEndedTopic כשהמשחק עדיין לא נגמר") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");

    bool called = false;
    bus.subscribe(GameSession::gameEndedTopic("room-a"), [&](const nlohmann::json&) { called = true; });

    session.tick(30);

    CHECK_FALSE(called);
}

TEST_CASE("GameSession - gameEndedTopic מתפרסם פעם אחת בלבד אחרי אכילת מלך, עם המנצח/מפסיד הנכונים") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::King, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {3, 4}), {3, 4});
    EventBus bus;
    GameSession session(board, bus, "room-a");
    session.setIdentity(RoomIdentity{ "alice", 1250, "bob", 1400, 0 });

    int callCount = 0;
    nlohmann::json received;
    bus.subscribe(GameSession::gameEndedTopic("room-a"), [&](const nlohmann::json& data) {
        ++callCount;
        received = data;
    });

    MoveResult result = session.engine().requestMove({3, 3}, {3, 4});
    REQUIRE(result.is_accepted);
    session.tick(1000);

    REQUIRE(callCount == 1);
    GameResult gameResult = GameResultCodec::decode(received);
    CHECK(gameResult.winner == Chess::Color::White);
    CHECK(gameResult.reason == GameEndReason::KingCapture);
    CHECK(gameResult.winnerUsername == "alice");
    CHECK(gameResult.winnerElo == 1250);
    CHECK(gameResult.loserUsername == "bob");
    CHECK(gameResult.loserElo == 1400);

    // A later tick must not publish a second time.
    session.tick(30);
    CHECK(callCount == 1);
}

TEST_CASE("GameSession - markDisconnectResign מפרסם gameEndedTopic עם הצד השני כמנצח, כולל שם/elo של שני הצדדים") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus, "room-a");
    // Winner's identity (White, still connected) comes from setIdentity;
    // loser's (Black, already disconnected by the time this fires in
    // production) is passed in directly - captured by the caller before
    // Black's ClientSession was erased.
    session.setIdentity(RoomIdentity{ "alice", 1250, "bob", 1400, 0 });

    nlohmann::json received;
    bus.subscribe(GameSession::gameEndedTopic("room-a"), [&](const nlohmann::json& data) { received = data; });

    session.markDisconnectResign(Chess::Color::Black, "bob", 1400);

    GameResult gameResult = GameResultCodec::decode(received);
    CHECK(gameResult.winner == Chess::Color::White);
    CHECK(gameResult.reason == GameEndReason::Disconnect);
    CHECK(gameResult.winnerUsername == "alice");
    CHECK(gameResult.winnerElo == 1250);
    CHECK(gameResult.loserUsername == "bob");
    CHECK(gameResult.loserElo == 1400);
}

TEST_CASE("GameSession - markDisconnectResign אחרי שהמלך כבר נאכל לא דורס את התוצאה האמיתית") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::King, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {3, 4}), {3, 4});
    EventBus bus;
    GameSession session(board, bus, "room-a");

    int callCount = 0;
    nlohmann::json received;
    bus.subscribe(GameSession::gameEndedTopic("room-a"), [&](const nlohmann::json& data) {
        ++callCount;
        received = data;
    });

    MoveResult result = session.engine().requestMove({3, 3}, {3, 4});
    REQUIRE(result.is_accepted);
    session.tick(1000); // king already captured here, published from inside tick()

    session.markDisconnectResign(Chess::Color::White, "alice", 1250); // must not override the real result

    REQUIRE(callCount == 1);
    GameResult gameResult = GameResultCodec::decode(received);
    CHECK(gameResult.winner == Chess::Color::White);
    CHECK(gameResult.reason == GameEndReason::KingCapture);
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
