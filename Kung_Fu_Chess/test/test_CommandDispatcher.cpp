#include "../doctest.h"
#include "../server/app/CommandDispatcher.h"
#include "../shared/model/Board.h"
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

    GameEngine makeEngine() {
        auto board = std::make_shared<Board>(8, 8);
        board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
        board->addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {7, 7}), {7, 7});
        return GameEngine(board);
    }
}

TEST_CASE("CommandDispatcher - MOVE תקין מהצבע הנכון מדווח הצלחה") {
    GameEngine engine = makeEngine();

    Message move;
    move.type = MessageType::Move;
    move.payload = { {"from", {{"row", 0}, {"col", 0}}}, {"to", {{"row", 0}, {"col", 1}}} };

    DispatchResult result = CommandDispatcher::dispatch(move, engine, Chess::Color::White);
    CHECK(result.success);
}

TEST_CASE("CommandDispatcher - JUMP תקין מהצבע הנכון מדווח הצלחה") {
    GameEngine engine = makeEngine();

    Message jump;
    jump.type = MessageType::Jump;
    jump.payload = { {"pos", {{"row", 0}, {"col", 0}}} };

    DispatchResult result = CommandDispatcher::dispatch(jump, engine, Chess::Color::White);
    CHECK(result.success);
}

TEST_CASE("CommandDispatcher - HELLO מדווח הצלחה גם לצופה") {
    GameEngine engine = makeEngine();

    Message hello;
    hello.type = MessageType::Hello;

    DispatchResult result = CommandDispatcher::dispatch(hello, engine, Chess::Color::None);
    CHECK(result.success);
}

TEST_CASE("CommandDispatcher - SNAPSHOT מלקוח נדחה") {
    GameEngine engine = makeEngine();

    Message snapshot;
    snapshot.type = MessageType::Snapshot;

    DispatchResult result = CommandDispatcher::dispatch(snapshot, engine, Chess::Color::White);
    CHECK_FALSE(result.success);
}

TEST_CASE("CommandDispatcher - MOVE עם payload חסר שדה לא מקריס ומחזיר כשלון") {
    GameEngine engine = makeEngine();

    Message move;
    move.type = MessageType::Move;
    move.payload = { {"from", {{"row", 0}, {"col", 0}}} }; // missing "to"

    DispatchResult result = CommandDispatcher::dispatch(move, engine, Chess::Color::White);
    CHECK_FALSE(result.success);
}

TEST_CASE("CommandDispatcher - MOVE עם טיפוס שדה שגוי לא מקריס ומחזיר כשלון") {
    GameEngine engine = makeEngine();

    Message move;
    move.type = MessageType::Move;
    move.payload = { {"from", {{"row", "not-a-number"}, {"col", 0}}}, {"to", {{"row", 0}, {"col", 1}}} };

    DispatchResult result = CommandDispatcher::dispatch(move, engine, Chess::Color::White);
    CHECK_FALSE(result.success);
}

// ==================== תפקידים (Stage C5) ====================

TEST_CASE("CommandDispatcher - MOVE מצופה (None) נדחה") {
    GameEngine engine = makeEngine();

    Message move;
    move.type = MessageType::Move;
    move.payload = { {"from", {{"row", 0}, {"col", 0}}}, {"to", {{"row", 0}, {"col", 1}}} };

    DispatchResult result = CommandDispatcher::dispatch(move, engine, Chess::Color::None);
    CHECK_FALSE(result.success);
}

TEST_CASE("CommandDispatcher - MOVE על כלי שאינו שייך לשולח נדחה") {
    GameEngine engine = makeEngine();

    Message move; // trying to move the White rook at (0,0) while claiming to be Black
    move.type = MessageType::Move;
    move.payload = { {"from", {{"row", 0}, {"col", 0}}}, {"to", {{"row", 0}, {"col", 1}}} };

    DispatchResult result = CommandDispatcher::dispatch(move, engine, Chess::Color::Black);
    CHECK_FALSE(result.success);
}

TEST_CASE("CommandDispatcher - JUMP מצופה (None) נדחה") {
    GameEngine engine = makeEngine();

    Message jump;
    jump.type = MessageType::Jump;
    jump.payload = { {"pos", {{"row", 0}, {"col", 0}}} };

    DispatchResult result = CommandDispatcher::dispatch(jump, engine, Chess::Color::None);
    CHECK_FALSE(result.success);
}

TEST_CASE("CommandDispatcher - JUMP על כלי שאינו שייך לשולח נדחה") {
    GameEngine engine = makeEngine();

    Message jump; // trying to jump the Black king at (7,7) while claiming to be White
    jump.type = MessageType::Jump;
    jump.payload = { {"pos", {{"row", 7}, {"col", 7}}} };

    DispatchResult result = CommandDispatcher::dispatch(jump, engine, Chess::Color::White);
    CHECK_FALSE(result.success);
}
