#include "../doctest.h"
#include "../server/input/MoveCommand.h"
#include "../server/engine/GameEngine.h"
#include "../shared/model/Board.h"
#include <memory>

class TestPiece : public Piece {
public:
    TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
        : Piece(id, color, kind, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

static std::shared_ptr<Piece> make(int id, Chess::Color color, Chess::Kind kind, Position pos) {
    return std::make_shared<TestPiece>(id, color, kind, pos);
}

TEST_CASE("MoveCommand::execute - שולח מהלך חוקי ל-GameEngine, הכלי מגיע ליעד") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    MoveCommand command({3, 3}, {3, 6});
    command.execute(engine);
    engine.wait(3000);

    auto snap = engine.snapshot();
    bool foundAtDestination = false;
    for (const auto& p : snap.pieces)
        if (p.cell == Position{3, 6}) foundAtDestination = true;
    CHECK(foundAtDestination);
}

TEST_CASE("MoveCommand::execute - מהלך לא חוקי לפי כללי הכלי לא משנה את הלוח") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    MoveCommand command({3, 3}, {4, 6}); // לא בקו ישר - לא חוקי לצריח
    command.execute(engine);
    engine.wait(3000);

    auto snap = engine.snapshot();
    bool stillAtOrigin = false;
    for (const auto& p : snap.pieces)
        if (p.cell == Position{3, 3}) stillAtOrigin = true;
    CHECK(stillAtOrigin);
}
