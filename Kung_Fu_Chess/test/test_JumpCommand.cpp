#include "../doctest.h"
#include "../input/JumpCommand.h"
#include "../engine/GameEngine.h"
#include "../model/Board.h"
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

TEST_CASE("JumpCommand::execute - שולח קפיצה ל-GameEngine, הכלי עובר למצב Jump בלי לזוז") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    JumpCommand command({3, 3});
    command.execute(engine);

    auto snap = engine.snapshot();
    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].state == Chess::State::Jump);
    CHECK(snap.pieces[0].cell == Position{3, 3});
}

TEST_CASE("JumpCommand::execute - קפיצה ממשבצת ריקה לא עושה כלום") {
    auto board = std::make_shared<Board>(8, 8);
    GameEngine engine(board);

    JumpCommand command({3, 3});
    command.execute(engine); // לא אמור לקרוס

    auto snap = engine.snapshot();
    CHECK(snap.pieces.empty());
}
