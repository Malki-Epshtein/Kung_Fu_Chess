#include "../doctest.h"
#include "../realtime/RealTimeArbiter.h"
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

// ==================== addMotion - זמן הגעה ====================

TEST_CASE("addMotion - תנועה של משבצת אחת מגיעה אחרי 1000 מ\"ש") {
    Board board(8, 8);
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 4}, 1);

    CHECK(arbiter.getActiveMotions().size() == 1);
    CHECK(arbiter.getActiveMotions()[0].arrival_time_ms == 1000);
}

TEST_CASE("addMotion - תנועה ישרה של שתי משבצות מגיעה אחרי 2000 מ\"ש") {
    Board board(8, 8);
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 5}, 1);

    CHECK(arbiter.getActiveMotions()[0].arrival_time_ms == 2000);
}

TEST_CASE("addMotion - תנועה אלכסונית נמדדת לפי צעדי-תא ולא לפי מרחק אווירי") {
    Board board(8, 8);
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({0, 0}, {3, 3}, 1); // שלוש משבצות אלכסוניות

    CHECK(arbiter.getActiveMotions()[0].arrival_time_ms == 3000);
}

TEST_CASE("addMotion - זמן ההגעה מצטבר מעל השעון הנוכחי") {
    Board board(8, 8);
    RealTimeArbiter arbiter(board);

    arbiter.tick(500);
    arbiter.addMotion({3, 3}, {3, 4}, 1);

    CHECK(arbiter.getActiveMotions()[0].arrival_time_ms == 1500);
}

// ==================== addJump ====================

TEST_CASE("addJump - יוצר תנועה שמקורה ויעדה זהים, מגיעה אחרי 1000 מ\"ש") {
    Board board(8, 8);
    RealTimeArbiter arbiter(board);

    arbiter.addJump({3, 3}, 1);

    CHECK(arbiter.getActiveMotions().size() == 1);
    CHECK(arbiter.getActiveMotions()[0].from == arbiter.getActiveMotions()[0].to);
    CHECK(arbiter.getActiveMotions()[0].arrival_time_ms == 1000);
}

// ==================== getClock ====================

TEST_CASE("getClock - מצטבר נכון על פני כמה קריאות tick") {
    Board board(8, 8);
    RealTimeArbiter arbiter(board);

    arbiter.tick(300);
    arbiter.tick(700);

    CHECK(arbiter.getClock() == 1000);
}

// ==================== tick - לפני הגעה ====================

TEST_CASE("tick - לפני זמן ההגעה הכלי נשאר לוגית במקורו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 4}, 1);
    arbiter.tick(999);

    CHECK(board.getPiece({3, 3})->getKind() == Chess::Kind::Rook);
    CHECK(board.isCellEmpty({3, 4}));
    CHECK(arbiter.getActiveMotions().size() == 1);
}

// ==================== tick - הגעה ====================

TEST_CASE("tick - בהגעה הכלי זז ליעד והתנועה מתפנה מהרשימה") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 4}, 1);
    arbiter.tick(1000);

    CHECK(board.isCellEmpty({3, 3}));
    CHECK(board.getPiece({3, 4})->getKind() == Chess::Kind::Rook);
    CHECK(arbiter.getActiveMotions().empty());
}

TEST_CASE("tick - שתי תנועות עם זמני הגעה שונים מתפנות בנפרד") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {2, 2}), {2, 2});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({0, 0}, {0, 1}, 1); // 1000 מ"ש
    arbiter.addMotion({2, 2}, {2, 5}, 2); // 3000 מ"ש

    arbiter.tick(1000);

    CHECK(arbiter.getActiveMotions().size() == 1);
    CHECK(board.getPiece({0, 1})->getKind() == Chess::Kind::Rook);
    CHECK(board.getPiece({2, 2})->getKind() == Chess::Kind::Rook);
}

// ==================== אירועי אכילה ותנאי ניצחון ====================

TEST_CASE("tick - אכילת כלי שאינו מלך מסירה אותו ומחזירה false") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {3, 6}), {3, 6});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 6}, 1);
    bool kingCaptured = arbiter.tick(3000);

    CHECK_FALSE(kingCaptured);
    CHECK(board.getPiece({3, 6})->getKind() == Chess::Kind::Rook);
    CHECK(board.isCellEmpty({3, 3}));
}

TEST_CASE("tick - אכילת מלך מחזירה true") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {3, 6}), {3, 6});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 6}, 1);
    bool kingCaptured = arbiter.tick(3000);

    CHECK(kingCaptured);
    CHECK(board.getPiece({3, 6})->getKind() == Chess::Kind::Rook);
}

// ==================== פתרון אטומי - הגעה בו-זמנית של תנועה וקפיצה ====================

TEST_CASE("tick - כלי נע שמגיע לתא של קפיצת אויב בו-זמנית נאכל באמצע הדרך") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook,   {3, 3}), {3, 3});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Knight, {3, 6}), {3, 6});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 6}, 1); // הרץ יגיע ב-3000 מ"ש (שעון עדיין 0)
    arbiter.tick(2000);                   // עדיין לא הגיע

    arbiter.addJump({3, 6}, 2);           // קפיצת האויב תגיע ב-2000+1000=3000, בו-זמנית עם הצריח

    bool kingCaptured = arbiter.tick(1000); // שעון מגיע ל-3000, שתי התנועות מתפתרות יחד

    CHECK_FALSE(kingCaptured);
    CHECK(board.isCellEmpty({3, 3}));  // הצריח הלבן נאכל באמצע הדרך, לא הגיע ליעד
    CHECK(board.getPiece({3, 6})->getKind() == Chess::Kind::Knight); // הפרש השחור נשאר במקומו
}
