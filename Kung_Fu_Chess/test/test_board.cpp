#include "../doctest.h"
#include <memory>
#include "../model/Board.h"

// כלי קונקרטי לצורך הטסטים
class TestPiece : public Piece {
public:
    TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
        : Piece(id, color, kind, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

TEST_CASE("Board - ממדי הלוח נגזרים נכון") {
    Board board(8, 8);
    CHECK(board.isWithinBounds({ 0, 0 }));    // תא פנימי תקין
    CHECK(board.isWithinBounds({ 7, 7 }));    // תא קצה תקין
    CHECK_FALSE(board.isWithinBounds({ 8, 0 }));   // מחוץ לגבולות
    CHECK_FALSE(board.isWithinBounds({ 0, 8 }));   // מחוץ לגבולות
}

TEST_CASE("Board - תאים ריקים מחזירים Kind::None") {
    Board board(8, 8);
    auto piece = board.getPiece({ 3, 3 });
    CHECK(piece != nullptr);
    CHECK(piece->getKind() == Chess::Kind::None);
    CHECK(board.isCellEmpty({ 3, 3 }));
}

TEST_CASE("Board - תאים תפוסים מחזירים את הכלי הנכון") {
    Board board(8, 8);
    auto rook = std::make_shared<TestPiece>(1, Chess::Color::White, Chess::Kind::Rook, Position{ 0, 0 });
    board.addPiece(rook, { 0, 0 });
    auto piece = board.getPiece({ 0, 0 });
    CHECK(piece->getKind() == Chess::Kind::Rook);
    CHECK(piece->getColor() == Chess::Color::White);
    CHECK(piece->getId() == 1);
}

TEST_CASE("Board - הוספת שני כלים לאותו תא נכשלת") {
    Board board(8, 8);
    auto p1 = std::make_shared<TestPiece>(1, Chess::Color::White, Chess::Kind::Rook,   Position{ 2, 2 });
    auto p2 = std::make_shared<TestPiece>(2, Chess::Color::Black, Chess::Kind::Bishop, Position{ 2, 2 });
    board.addPiece(p1, { 2, 2 });
    board.addPiece(p2, { 2, 2 }); // הכלי השני דורס את הראשון - לא אמור לקרות
    CHECK(board.getPiece({ 2, 2 })->getId() == 1); // הכלי המקורי נשמר
}

TEST_CASE("Board - הזזת כלי מעדכנת מקור ויעד") {
    Board board(8, 8);
    auto knight = std::make_shared<TestPiece>(3, Chess::Color::Black, Chess::Kind::Knight, Position{ 1, 1 });
    board.addPiece(knight, { 1, 1 });
    board.movePiece({ 1, 1 }, { 3, 2 });
    CHECK(board.isCellEmpty({ 1, 1 }));
    CHECK(board.getPiece({ 3, 2 })->getKind() == Chess::Kind::Knight);
}

TEST_CASE("Board - הסרת כלי שנאכל מנקה את התא") {
    Board board(8, 8);
    auto queen = std::make_shared<TestPiece>(4, Chess::Color::White, Chess::Kind::Queen, Position{ 4, 4 });
    board.addPiece(queen, { 4, 4 });
    board.removePiece({ 4, 4 });
    CHECK(board.isCellEmpty({ 4, 4 }));
    CHECK(board.getPiece({ 4, 4 })->getKind() == Chess::Kind::None);
}
