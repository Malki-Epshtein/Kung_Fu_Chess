#include "../doctest.h"
#include "../rules/PieceRules.h"
#include "../rules/RookRules.h"
#include "../rules/BishopRules.h"
#include "../rules/QueenRules.h"
#include "../rules/KnightRules.h"
#include "../rules/KingRules.h"
#include "../rules/PawnRules.h"
#include "../model/Board.h"
#include "../model/Piece.h"
#include <algorithm>

// כלי עזר לטסטים
class TestPiece : public Piece {
public:
    TestPiece(Chess::Color c, Chess::Kind k, Position pos)
        : Piece(1, c, k, pos, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

static std::shared_ptr<Piece> make(Chess::Color c, Chess::Kind k, Position pos) {
    return std::make_shared<TestPiece>(c, k, pos);
}

static bool contains(const std::vector<Position>& v, Position p) {
    return std::find(v.begin(), v.end(), p) != v.end();
}

// ==================== ROOK ====================

TEST_CASE("Rook - לוח ריק, מהמרכז - 14 תנועות") {
    Board board(8, 8);
    auto rook = make(Chess::Color::White, Chess::Kind::Rook, {3, 3});
    board.addPiece(rook, {3, 3});
    auto moves = RookRules::moves(board, *rook);
    CHECK(moves.size() == 14);
}

TEST_CASE("Rook - חסום על ידי כלי ידידותי") {
    Board board(8, 8);
    auto rook = make(Chess::Color::White, Chess::Kind::Rook, {3, 3});
    auto friend1 = make(Chess::Color::White, Chess::Kind::Pawn, {3, 5});
    board.addPiece(rook, {3, 3});
    board.addPiece(friend1, {3, 5});
    auto moves = RookRules::moves(board, *rook);
    CHECK_FALSE(contains(moves, {3, 5})); // לא יכול לדרוס ידיד
    CHECK_FALSE(contains(moves, {3, 6})); // חסום מעבר לידיד
    CHECK(contains(moves, {3, 4}));       // יכול לגשת לפני הידיד
}

TEST_CASE("Rook - אוכל כלי אויב ועוצר") {
    Board board(8, 8);
    auto rook = make(Chess::Color::White, Chess::Kind::Rook, {3, 3});
    auto enemy = make(Chess::Color::Black, Chess::Kind::Pawn, {3, 5});
    board.addPiece(rook, {3, 3});
    board.addPiece(enemy, {3, 5});
    auto moves = RookRules::moves(board, *rook);
    CHECK(contains(moves, {3, 5}));       // יכול לאכול
    CHECK_FALSE(contains(moves, {3, 6})); // לא יכול לעבור מעבר לאויב
}

TEST_CASE("Rook - בפינה, רק 2 כיוונים") {
    Board board(8, 8);
    auto rook = make(Chess::Color::White, Chess::Kind::Rook, {0, 0});
    board.addPiece(rook, {0, 0});
    auto moves = RookRules::moves(board, *rook);
    CHECK(moves.size() == 14); // 7 ימינה + 7 למטה
}

TEST_CASE("Rook - מוקף ידידים מכל צד") {
    Board board(8, 8);
    auto rook = make(Chess::Color::White, Chess::Kind::Rook, {3, 3});
    board.addPiece(rook, {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 4}), {3, 4});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 2}), {3, 2});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {2, 3}), {2, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    auto moves = RookRules::moves(board, *rook);
    CHECK(moves.empty());
}

// ==================== BISHOP ====================

TEST_CASE("Bishop - לוח ריק מהמרכז - 13 תנועות") {
    Board board(8, 8);
    auto bishop = make(Chess::Color::White, Chess::Kind::Bishop, {3, 3});
    board.addPiece(bishop, {3, 3});
    auto moves = BishopRules::moves(board, *bishop);
    CHECK(moves.size() == 13);
}

TEST_CASE("Bishop - חסום על ידי ידיד באלכסון") {
    Board board(8, 8);
    auto bishop = make(Chess::Color::White, Chess::Kind::Bishop, {3, 3});
    auto friend1 = make(Chess::Color::White, Chess::Kind::Pawn, {5, 5});
    board.addPiece(bishop, {3, 3});
    board.addPiece(friend1, {5, 5});
    auto moves = BishopRules::moves(board, *bishop);
    CHECK(contains(moves, {4, 4}));
    CHECK_FALSE(contains(moves, {5, 5}));
    CHECK_FALSE(contains(moves, {6, 6}));
}

TEST_CASE("Bishop - אוכל אויב באלכסון") {
    Board board(8, 8);
    auto bishop = make(Chess::Color::White, Chess::Kind::Bishop, {3, 3});
    auto enemy = make(Chess::Color::Black, Chess::Kind::Pawn, {5, 5});
    board.addPiece(bishop, {3, 3});
    board.addPiece(enemy, {5, 5});
    auto moves = BishopRules::moves(board, *bishop);
    CHECK(contains(moves, {5, 5}));
    CHECK_FALSE(contains(moves, {6, 6}));
}

TEST_CASE("Bishop - בפינה") {
    Board board(8, 8);
    auto bishop = make(Chess::Color::White, Chess::Kind::Bishop, {0, 0});
    board.addPiece(bishop, {0, 0});
    auto moves = BishopRules::moves(board, *bishop);
    CHECK(moves.size() == 7); // רק אלכסון אחד
}

// ==================== QUEEN ====================

TEST_CASE("Queen - לוח ריק מהמרכז - 27 תנועות") {
    Board board(8, 8);
    auto queen = make(Chess::Color::White, Chess::Kind::Queen, {3, 3});
    board.addPiece(queen, {3, 3});
    auto moves = QueenRules::moves(board, *queen);
    CHECK(moves.size() == 27);
}

TEST_CASE("Queen - חסום בכל הכיוונים") {
    Board board(8, 8);
    auto queen = make(Chess::Color::White, Chess::Kind::Queen, {3, 3});
    board.addPiece(queen, {3, 3});
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3+dr, 3+dc}), {3+dr, 3+dc});
        }
    auto moves = QueenRules::moves(board, *queen);
    CHECK(moves.empty());
}

TEST_CASE("Queen - אוכלת אויבים בכל הכיוונים") {
    Board board(8, 8);
    auto queen = make(Chess::Color::White, Chess::Kind::Queen, {3, 3});
    board.addPiece(queen, {3, 3});
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            board.addPiece(make(Chess::Color::Black, Chess::Kind::Pawn, {3+dr, 3+dc}), {3+dr, 3+dc});
        }
    auto moves = QueenRules::moves(board, *queen);
    CHECK(moves.size() == 8); // אוכלת 8 אויבים, לא עוברת מעבר
}

// ==================== KNIGHT ====================

TEST_CASE("Knight - מהמרכז - 8 תנועות") {
    Board board(8, 8);
    auto knight = make(Chess::Color::White, Chess::Kind::Knight, {3, 3});
    board.addPiece(knight, {3, 3});
    auto moves = KnightRules::moves(board, *knight);
    CHECK(moves.size() == 8);
}

TEST_CASE("Knight - מהפינה - 2 תנועות") {
    Board board(8, 8);
    auto knight = make(Chess::Color::White, Chess::Kind::Knight, {0, 0});
    board.addPiece(knight, {0, 0});
    auto moves = KnightRules::moves(board, *knight);
    CHECK(moves.size() == 2);
    CHECK(contains(moves, {1, 2}));
    CHECK(contains(moves, {2, 1}));
}

TEST_CASE("Knight - קופץ מעל כלים") {
    Board board(8, 8);
    auto knight = make(Chess::Color::White, Chess::Kind::Knight, {3, 3});
    board.addPiece(knight, {3, 3});
    // מלא ידידים סביב - הפרש עדיין קופץ
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++)
            if (dr != 0 || dc != 0)
                board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3+dr, 3+dc}), {3+dr, 3+dc});
    auto moves = KnightRules::moves(board, *knight);
    CHECK(moves.size() == 8); // עדיין 8 קפיצות
}

TEST_CASE("Knight - לא יכול לנחות על ידיד") {
    Board board(8, 8);
    auto knight = make(Chess::Color::White, Chess::Kind::Knight, {3, 3});
    auto friend1 = make(Chess::Color::White, Chess::Kind::Pawn, {1, 2});
    board.addPiece(knight, {3, 3});
    board.addPiece(friend1, {1, 2});
    auto moves = KnightRules::moves(board, *knight);
    CHECK_FALSE(contains(moves, {1, 2}));
}

TEST_CASE("Knight - יכול לאכול אויב") {
    Board board(8, 8);
    auto knight = make(Chess::Color::White, Chess::Kind::Knight, {3, 3});
    auto enemy = make(Chess::Color::Black, Chess::Kind::Pawn, {1, 2});
    board.addPiece(knight, {3, 3});
    board.addPiece(enemy, {1, 2});
    auto moves = KnightRules::moves(board, *knight);
    CHECK(contains(moves, {1, 2}));
}

// ==================== KING ====================

TEST_CASE("King - מהמרכז - 8 תנועות") {
    Board board(8, 8);
    auto king = make(Chess::Color::White, Chess::Kind::King, {3, 3});
    board.addPiece(king, {3, 3});
    auto moves = KingRules::moves(board, *king);
    CHECK(moves.size() == 8);
}

TEST_CASE("King - מהפינה - 3 תנועות") {
    Board board(8, 8);
    auto king = make(Chess::Color::White, Chess::Kind::King, {0, 0});
    board.addPiece(king, {0, 0});
    auto moves = KingRules::moves(board, *king);
    CHECK(moves.size() == 3);
}

TEST_CASE("King - לא יכול לנחות על ידיד") {
    Board board(8, 8);
    auto king = make(Chess::Color::White, Chess::Kind::King, {3, 3});
    board.addPiece(king, {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 4}), {3, 4});
    auto moves = KingRules::moves(board, *king);
    CHECK_FALSE(contains(moves, {3, 4}));
}

TEST_CASE("King - יכול לאכול אויב") {
    Board board(8, 8);
    auto king = make(Chess::Color::White, Chess::Kind::King, {3, 3});
    auto enemy = make(Chess::Color::Black, Chess::Kind::Pawn, {3, 4});
    board.addPiece(king, {3, 3});
    board.addPiece(enemy, {3, 4});
    auto moves = KingRules::moves(board, *king);
    CHECK(contains(moves, {3, 4}));
}

// ==================== PAWN ====================

TEST_CASE("Pawn לבן - צעד אחד קדימה") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {4, 3});
    board.addPiece(pawn, {4, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK(contains(moves, {3, 3}));
}

TEST_CASE("Pawn לבן - שני צעדים מהשורה ההתחלתית") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {6, 3});
    board.addPiece(pawn, {6, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK(contains(moves, {5, 3}));
    CHECK(contains(moves, {4, 3}));
}

TEST_CASE("Pawn לבן - לא שני צעדים אם לא בשורת התחלה") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {4, 3});
    board.addPiece(pawn, {4, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK_FALSE(contains(moves, {2, 3}));
}

TEST_CASE("Pawn לבן - חסום קדימה") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {4, 3});
    auto blocker = make(Chess::Color::Black, Chess::Kind::Pawn, {3, 3});
    board.addPiece(pawn, {4, 3});
    board.addPiece(blocker, {3, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK_FALSE(contains(moves, {3, 3}));
}

TEST_CASE("Pawn לבן - אוכל באלכסון") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {4, 3});
    auto enemy = make(Chess::Color::Black, Chess::Kind::Pawn, {3, 4});
    board.addPiece(pawn, {4, 3});
    board.addPiece(enemy, {3, 4});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK(contains(moves, {3, 4}));
}

TEST_CASE("Pawn לבן - לא אוכל קדימה") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {4, 3});
    auto enemy = make(Chess::Color::Black, Chess::Kind::Pawn, {3, 3});
    board.addPiece(pawn, {4, 3});
    board.addPiece(enemy, {3, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK_FALSE(contains(moves, {3, 3})); // לא אוכל קדימה
}

TEST_CASE("Pawn לבן - לא אוכל ידיד באלכסון") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::White, Chess::Kind::Pawn, {4, 3});
    auto friend1 = make(Chess::Color::White, Chess::Kind::Pawn, {3, 4});
    board.addPiece(pawn, {4, 3});
    board.addPiece(friend1, {3, 4});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK_FALSE(contains(moves, {3, 4}));
}

TEST_CASE("Pawn שחור - זז למטה") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::Black, Chess::Kind::Pawn, {3, 3});
    board.addPiece(pawn, {3, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK(contains(moves, {4, 3}));
}

TEST_CASE("Pawn שחור - שני צעדים מהשורה ההתחלתית") {
    Board board(8, 8);
    auto pawn = make(Chess::Color::Black, Chess::Kind::Pawn, {1, 3});
    board.addPiece(pawn, {1, 3});
    auto moves = PawnRules::moves(board, *pawn);
    CHECK(contains(moves, {2, 3}));
    CHECK(contains(moves, {3, 3}));
}

// ==================== PIECERULES (מנתב) ====================

TEST_CASE("PieceRules - מנתב נכון לצריח") {
    Board board(8, 8);
    auto rook = make(Chess::Color::White, Chess::Kind::Rook, {3, 3});
    board.addPiece(rook, {3, 3});
    auto moves = PieceRules::legalDestinations(board, *rook);
    CHECK(moves.size() == 14);
}

TEST_CASE("PieceRules - מנתב נכון לפרש") {
    Board board(8, 8);
    auto knight = make(Chess::Color::White, Chess::Kind::Knight, {3, 3});
    board.addPiece(knight, {3, 3});
    auto moves = PieceRules::legalDestinations(board, *knight);
    CHECK(moves.size() == 8);
}

TEST_CASE("PieceRules - EmptyPiece מחזיר ריק") {
    Board board(8, 8);
    EmptyPiece empty({3, 3});
    auto moves = PieceRules::legalDestinations(board, empty);
    CHECK(moves.empty());
}
