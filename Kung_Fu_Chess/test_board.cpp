#include "doctest.h"
#include <memory>
#include "model/Board.h"

// כלי קונקרטי לצורך הטסטים
class TestPiece : public Piece {
public:
    TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
        : Piece(id, color, kind, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

// 1. ממדי הלוח נגזרים נכון
void test_board_dimensions() {
    Board board(8, 8);
    assert(board.isWithinBounds({ 0, 0 }));    // תא פנימי תקין
    assert(board.isWithinBounds({ 7, 7 }));    // תא קצה תקין
    assert(!board.isWithinBounds({ 8, 0 }));   // מחוץ לגבולות
    assert(!board.isWithinBounds({ 0, 8 }));   // מחוץ לגבולות
}

// 2. תאים ריקים מחזירים Kind::None
void test_empty_cell_returns_none() {
    Board board(8, 8);
    auto piece = board.getPiece({ 3, 3 });
    assert(piece != nullptr);
    assert(piece->getKind() == Chess::Kind::None);
    assert(board.isCellEmpty({ 3, 3 }));
}

// 3. תאים תפוסים מחזירים את הכלי הנכון
void test_occupied_cell_returns_correct_piece() {
    Board board(8, 8);
    auto rook = std::make_shared<TestPiece>(1, Chess::Color::White, Chess::Kind::Rook, Position{ 0, 0 });
    board.addPiece(rook, { 0, 0 });
    auto piece = board.getPiece({ 0, 0 });
    assert(piece->getKind() == Chess::Kind::Rook);
    assert(piece->getColor() == Chess::Color::White);
    assert(piece->getId() == 1);
}

// 4. הוספת שני כלים לאותו תא נכשלת
void test_add_two_pieces_same_cell_fails() {
    Board board(8, 8);
    auto p1 = std::make_shared<TestPiece>(1, Chess::Color::White, Chess::Kind::Rook,   Position{ 2, 2 });
    auto p2 = std::make_shared<TestPiece>(2, Chess::Color::Black, Chess::Kind::Bishop, Position{ 2, 2 });
    board.addPiece(p1, { 2, 2 });
    board.addPiece(p2, { 2, 2 }); // הכלי השני דורס את הראשון - לא אמור לקרות
    assert(board.getPiece({ 2, 2 })->getId() == 1); // הכלי המקורי נשמר
}

// 5. הזזת כלי מעדכנת מקור ויעד
void test_move_piece_updates_source_and_dest() {
    Board board(8, 8);
    auto knight = std::make_shared<TestPiece>(3, Chess::Color::Black, Chess::Kind::Knight, Position{ 1, 1 });
    board.addPiece(knight, { 1, 1 });
    board.movePiece({ 1, 1 }, { 3, 2 });
    assert(board.isCellEmpty({ 1, 1 }));
    assert(board.getPiece({ 3, 2 })->getKind() == Chess::Kind::Knight);
}

// 6. הסרת כלי שנאכל מנקה את התא
void test_remove_captured_piece_clears_cell() {
    Board board(8, 8);
    auto queen = std::make_shared<TestPiece>(4, Chess::Color::White, Chess::Kind::Queen, Position{ 4, 4 });
    board.addPiece(queen, { 4, 4 });
    board.removePiece({ 4, 4 });
    assert(board.isCellEmpty({ 4, 4 }));
    assert(board.getPiece({ 4, 4 })->getKind() == Chess::Kind::None);
}


