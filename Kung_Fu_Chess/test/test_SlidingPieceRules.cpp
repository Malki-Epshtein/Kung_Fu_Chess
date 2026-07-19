#include "../doctest.h"
#include "../server/rules/SlidingPieceRules.h"
#include "../shared/model/Board.h"
#include <memory>
#include <algorithm>

class TestPiece : public Piece {
public:
    TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
        : Piece(id, color, kind, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

static std::shared_ptr<Piece> make(int id, Chess::Color color, Chess::Kind kind, Position pos) {
    return std::make_shared<TestPiece>(id, color, kind, pos);
}

static bool contains(const std::vector<Position>& v, Position p) {
    return std::find(v.begin(), v.end(), p) != v.end();
}

TEST_CASE("SlidingPieceRules::slide - נע בכיוון אחד עד קצה הלוח כשאין חוסמים") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    auto piece = board.getPiece({0, 0});

    auto result = SlidingPieceRules::slide(board, *piece, { {0, 1} }, false);

    CHECK(result.size() == 7);
    CHECK(contains(result, {0, 7}));
}

TEST_CASE("SlidingPieceRules::slide - נעצר לפני כלי ידידותי, לא כולל אותו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {0, 3}), {0, 3});
    auto piece = board.getPiece({0, 0});

    auto result = SlidingPieceRules::slide(board, *piece, { {0, 1} }, false);

    CHECK(result.size() == 2);
    CHECK(contains(result, {0, 1}));
    CHECK(contains(result, {0, 2}));
    CHECK_FALSE(contains(result, {0, 3}));
}

TEST_CASE("SlidingPieceRules::slide - עוצר על כלי אויב וכולל אותו כיעד אכילה, לא ממשיך מעבר") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {0, 3}), {0, 3});
    auto piece = board.getPiece({0, 0});

    auto result = SlidingPieceRules::slide(board, *piece, { {0, 1} }, false);

    CHECK(result.size() == 3);
    CHECK(contains(result, {0, 3}));
    CHECK_FALSE(contains(result, {0, 4}));
}

TEST_CASE("SlidingPieceRules::slide - ignoreBlockers ממשיך דרך כל חוסם עד קצה הלוח") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {0, 3}), {0, 3});
    board.addPiece(make(3, Chess::Color::White, Chess::Kind::Pawn, {0, 5}), {0, 5});
    auto piece = board.getPiece({0, 0});

    auto result = SlidingPieceRules::slide(board, *piece, { {0, 1} }, true);

    CHECK(result.size() == 7);
    CHECK(contains(result, {0, 3}));
    CHECK(contains(result, {0, 5}));
    CHECK(contains(result, {0, 7}));
}

TEST_CASE("SlidingPieceRules::slide - כמה כיוונים בבת אחת (למשל מלכה)") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Queen, {4, 4}), {4, 4});
    auto piece = board.getPiece({4, 4});

    auto result = SlidingPieceRules::slide(board, *piece, { {1, 0}, {0, 1} }, false);

    CHECK(contains(result, {5, 4}));
    CHECK(contains(result, {4, 5}));
    CHECK_FALSE(contains(result, {3, 4}));
}
