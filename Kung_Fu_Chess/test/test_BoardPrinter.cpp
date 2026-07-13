#include "../doctest.h"
#include "../io/BoardPrinter.h"
#include "../model/Board.h"
#include "../model/GameState.h"
#include <sstream>
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

// ==================== print(Board) ====================

TEST_CASE("print(GameState) - לוח ריק מודפס כנקודות בלבד") {
    GameState state(std::make_shared<Board>(3, 2));
    std::ostringstream out;
    BoardPrinter::print(state, out);
    CHECK(out.str() == ". . .\n. . .\n");
}

TEST_CASE("print(GameState) - כלי בודד מודפס בסימון הנכון") {
    auto board = std::make_shared<Board>(3, 3);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::King, {1, 1}), {1, 1});
    GameState state(board);
    std::ostringstream out;
    BoardPrinter::print(state, out);
    CHECK(out.str() == ". . .\n. wK .\n. . .\n");
}

TEST_CASE("print(GameState) - דוגמת המסמך: לוח שלוש-על-שלוש עם כמה כלים") {
    auto board = std::make_shared<Board>(3, 3);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::King,   {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook,   {0, 2}), {0, 2});
    board->addPiece(make(3, Chess::Color::White, Chess::Kind::Knight, {2, 1}), {2, 1});
    board->addPiece(make(4, Chess::Color::Black, Chess::Kind::King,   {2, 2}), {2, 2});
    GameState state(board);

    std::ostringstream out;
    BoardPrinter::print(state, out);
    CHECK(out.str() == "wK . bR\n. . .\n. wN bK\n");
}

TEST_CASE("print(GameState) - כל סוגי הכלים משני הצבעים מודפסים באות הנכונה") {
    auto board = std::make_shared<Board>(6, 2);
    board->addPiece(make(1,  Chess::Color::White, Chess::Kind::Pawn,   {0, 0}), {0, 0});
    board->addPiece(make(2,  Chess::Color::White, Chess::Kind::Rook,   {0, 1}), {0, 1});
    board->addPiece(make(3,  Chess::Color::White, Chess::Kind::Knight, {0, 2}), {0, 2});
    board->addPiece(make(4,  Chess::Color::White, Chess::Kind::Bishop, {0, 3}), {0, 3});
    board->addPiece(make(5,  Chess::Color::White, Chess::Kind::Queen,  {0, 4}), {0, 4});
    board->addPiece(make(6,  Chess::Color::White, Chess::Kind::King,   {0, 5}), {0, 5});
    board->addPiece(make(7,  Chess::Color::Black, Chess::Kind::Pawn,   {1, 0}), {1, 0});
    board->addPiece(make(8,  Chess::Color::Black, Chess::Kind::Rook,   {1, 1}), {1, 1});
    board->addPiece(make(9,  Chess::Color::Black, Chess::Kind::Knight, {1, 2}), {1, 2});
    board->addPiece(make(10, Chess::Color::Black, Chess::Kind::Bishop, {1, 3}), {1, 3});
    board->addPiece(make(11, Chess::Color::Black, Chess::Kind::Queen,  {1, 4}), {1, 4});
    board->addPiece(make(12, Chess::Color::Black, Chess::Kind::King,   {1, 5}), {1, 5});
    GameState state(board);

    std::ostringstream out;
    BoardPrinter::print(state, out);
    CHECK(out.str() == "wP wR wN wB wQ wK\nbP bR bN bB bQ bK\n");
}

// ==================== print(GameSnapshot) ====================

TEST_CASE("print(GameSnapshot) - לוח ריק מודפס כנקודות בלבד") {
    GameSnapshot snap;
    snap.board_width  = 3;
    snap.board_height = 2;
    snap.game_over     = false;

    std::ostringstream out;
    BoardPrinter::print(snap, out);
    CHECK(out.str() == ". . .\n. . .\n");
}

TEST_CASE("print(GameSnapshot) - מזהה זהה בפלט ל-print(GameState) עבור אותו מצב") {
    auto board = std::make_shared<Board>(3, 3);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::King, {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {0, 2}), {0, 2});
    GameState state(board);

    std::ostringstream boardOut;
    BoardPrinter::print(state, boardOut);

    GameSnapshot snap;
    snap.board_width  = 3;
    snap.board_height = 3;
    snap.game_over     = false;
    snap.pieces.push_back({Chess::Kind::King, Chess::Color::White, {0, 0}, Chess::State::Idle});
    snap.pieces.push_back({Chess::Kind::Rook, Chess::Color::Black, {0, 2}, Chess::State::Idle});

    std::ostringstream snapOut;
    BoardPrinter::print(snap, snapOut);

    CHECK(boardOut.str() == snapOut.str());
}
