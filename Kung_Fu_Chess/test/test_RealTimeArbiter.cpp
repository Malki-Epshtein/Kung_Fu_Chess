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

// ==================== isPieceBusy / isPieceCoolingDown ====================

TEST_CASE("isPieceBusy - true בזמן שהתנועה פעילה, false אחרי שהיא נפתרת") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    RealTimeArbiter arbiter(board);

    CHECK_FALSE(arbiter.isPieceBusy(1));

    arbiter.addMotion({3, 3}, {3, 4}, 1);
    CHECK(arbiter.isPieceBusy(1));

    arbiter.tick(1000);
    CHECK_FALSE(arbiter.isPieceBusy(1));
}

TEST_CASE("isPieceCoolingDown - true מיד אחרי ההגעה, false אחרי שחלף זמן הצינון") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    RealTimeArbiter arbiter(board);

    arbiter.addMotion({3, 3}, {3, 4}, 1);
    arbiter.tick(1000); // הגעה

    CHECK(arbiter.isPieceCoolingDown(1));

    arbiter.tick(999);
    CHECK(arbiter.isPieceCoolingDown(1));

    arbiter.tick(1);
    CHECK_FALSE(arbiter.isPieceCoolingDown(1));
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

// ==================== collisionEnabled - פרש הורג ידידותי בהגעה (סעיף 8) ====================

TEST_CASE("tick - collisionEnabled: כלי לא-פרש שמגיע ליעד תפוס בכלי ידידותי נכשל בשקט") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Bishop, {1, 3}), {1, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 3}, 1); // צריח: 3 צעדים, יגיע ב-t=3000
    arbiter.addMotion({1, 3}, {0, 3}, 2); // רץ: צעד אחד, יגיע ב-t=1000, לאותו יעד

    arbiter.tick(1000); // הרץ מגיע ל-(0,3), עדיין ריק באותו רגע
    CHECK(board.getPiece({0, 3})->getId() == 2);

    bool kingCaptured = arbiter.tick(2000); // t=3000 - הצריח מגיע ל-(0,3) שכבר תפוס בידיד

    CHECK_FALSE(kingCaptured);
    CHECK(board.getPiece({0, 0})->getId() == 1);  // הצריח נכשל בשקט - נשאר במקורו
    CHECK(board.getPiece({0, 3})->getId() == 2);  // הרץ עדיין שם, לא נהרג
}

TEST_CASE("tick - collisionEnabled: פרש שמגיע ליעד תפוס בכלי ידידותי הורג אותו ומשלים את המהלך") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Knight, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Bishop, {1, 3}), {1, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 3}, 1); // הפרש: 3 צעדים, יגיע ב-t=3000
    arbiter.addMotion({1, 3}, {0, 3}, 2); // רץ: צעד אחד, יגיע ב-t=1000

    arbiter.tick(1000);
    CHECK(board.getPiece({0, 3})->getId() == 2);

    bool kingCaptured = arbiter.tick(2000); // t=3000 - הפרש מגיע ליעד התפוס

    CHECK_FALSE(kingCaptured);
    CHECK(board.isCellEmpty({0, 0}));                                  // הפרש עזב את מקורו
    CHECK(board.getPiece({0, 3})->getKind() == Chess::Kind::Knight);   // הפרש במקום, הרץ הידידותי נהרג
}

TEST_CASE("tick - כשcollisionEnabled כבוי (ברירת מחדל), movePiece הישן ממשיך לדרוס כל דבר כמו קודם") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Bishop, {1, 3}), {1, 3});
    RealTimeArbiter arbiter(board); // ברירת מחדל: collisionEnabled=false

    arbiter.addMotion({0, 0}, {0, 3}, 1);
    arbiter.addMotion({1, 3}, {0, 3}, 2);

    arbiter.tick(1000);
    arbiter.tick(2000);

    CHECK(board.getPiece({0, 3})->getKind() == Chess::Kind::Rook); // ההתנהגות הישנה, ללא שינוי
    CHECK(board.isCellEmpty({0, 0}));
}

// ==================== collisionEnabled - התנגשות אויבים במסלול ====================

TEST_CASE("tick - כשcollisionEnabled כבוי (ברירת מחדל), מסלולים חוצים של אויבים לא גורמים להתנגשות") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {0, 3}), {0, 3});
    RealTimeArbiter arbiter(board); // ברירת מחדל: collisionEnabled=false

    arbiter.addMotion({0, 0}, {0, 5}, 1);
    arbiter.tick(3000);
    arbiter.addMotion({0, 3}, {0, 4}, 2);

    arbiter.tick(1000); // t=4000 - שני המסלולים חולפים דרך (0,4) בו-זמנית, בלי collisionEnabled

    bool whiteStillActive = false;
    for (const auto& m : arbiter.getActiveMotions())
        if (m.piece_id == 1) whiteStillActive = true;

    CHECK(whiteStillActive); // הלבן ממשיך כרגיל
    CHECK(board.getPiece({0, 4})->getColor() == Chess::Color::Black); // השחור הגיע כרגיל ליעדו, לא נאכל
}

TEST_CASE("tick - התנגשות אויבים: מי שהתחיל לזוז קודם מנצח, גם אם הוא איטי יותר ומגיע בפועל מאוחר יותר") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {0, 3}), {0, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 5}, 1); // לבן: התחיל ב-t=0, יגיע ל-(0,4) ב-t=4000
    arbiter.tick(3000);                  // עכשיו t=3000

    arbiter.addMotion({0, 3}, {0, 4}, 2); // שחור: התחיל מאוחר יותר (t=3000), גם יגיע ל-(0,4) ב-t=4000

    bool kingCaptured = arbiter.tick(1000); // t=4000 - שתי התנועות נפגשות ב-(0,4) בו-זמנית

    CHECK_FALSE(kingCaptured);
    CHECK(board.isCellEmpty({0, 3})); // השחור (התחיל מאוחר) נאכל באמצע הדרך

    bool whiteStillActive = false;
    for (const auto& m : arbiter.getActiveMotions())
        if (m.piece_id == 1) whiteStillActive = true;
    CHECK(whiteStillActive); // הלבן (התחיל קודם) ממשיך בלי הפרעה, עדיין לא הגיע ליעדו הסופי (0,5)
}

TEST_CASE("tick - התנגשות אויבים: אם המפסיד הוא מלך, זה נחשב אכילת מלך") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {0, 3}), {0, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 5}, 1);
    arbiter.tick(3000);
    arbiter.addMotion({0, 3}, {0, 4}, 2); // המלך השחור מתחיל מאוחר יותר - יפסיד

    bool kingCaptured = arbiter.tick(1000);
    CHECK(kingCaptured);
}

TEST_CASE("tick - התנגשות אויבים: פרש פטור מהתנגשות ועובר דרך התא בלי להיפגע") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Knight, {0, 3}), {0, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 5}, 1);
    arbiter.tick(3000);
    arbiter.addMotion({0, 3}, {0, 4}, 2); // "פרש" - פטור מכללי ההתנגשות

    bool kingCaptured = arbiter.tick(1000);

    CHECK_FALSE(kingCaptured);
    bool blackStillActive = false;
    for (const auto& m : arbiter.getActiveMotions())
        if (m.piece_id == 2) blackStillActive = true;
    CHECK_FALSE(blackStillActive); // הפרש כבר הגיע ליעדו כרגיל (מהלך של תא בודד), לא נאכל
    CHECK(board.getPiece({0, 4})->getKind() == Chess::Kind::Knight);
}

// ==================== collisionEnabled - near-miss בין כלים ידידותיים ====================

TEST_CASE("addMotion - near-miss: כלי ידידותי שהיה מגיע יותר מאוחר לתא המשותף נעצר משבצת אחת לפניו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Queen, {5, 3}), {5, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 5}, 1); // מגיע ל-(0,3) ב-t=3000, ל-(0,4) ב-t=4000 וכו'
    bool started = arbiter.addMotion({5, 3}, {0, 3}, 2); // בלי קיצור, היה מגיע ל-(0,3) ב-t=5000 - יותר מאוחר

    CHECK(started);

    bool found = false;
    for (const auto& m : arbiter.getActiveMotions()) {
        if (m.piece_id == 2) {
            found = true;
            CHECK(m.to == Position{1, 3});        // נעצר משבצת אחת לפני (0,3)
            CHECK(m.arrival_time_ms == 4000);
        }
    }
    CHECK(found);
}

TEST_CASE("addMotion - near-miss: אם הצעד הראשון כבר תפוס עי מסלול ידידותי, המהלך נדחה לגמרי") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Queen, {1, 1}), {1, 1});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 5}, 1); // עובר דרך (0,1) ב-t=1000
    bool started = arbiter.addMotion({1, 1}, {0, 1}, 2); // הצעד הראשון שלו הוא בדיוק (0,1), גם ב-t=1000

    CHECK_FALSE(started);
    CHECK_FALSE(arbiter.isPieceBusy(2)); // לא נוסף שום Motion עבור כלי 2
}

TEST_CASE("addMotion - near-miss: פרש פטור מהכלל, לא נעצר גם כשמסלולו חוצה כלי ידידותי") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Knight, {5, 3}), {5, 3});
    RealTimeArbiter arbiter(board, /*collisionEnabled=*/true);

    arbiter.addMotion({0, 0}, {0, 5}, 1);
    bool started = arbiter.addMotion({5, 3}, {0, 3}, 2); // "פרש" - פטור מ-near-miss

    CHECK(started);
    bool found = false;
    for (const auto& m : arbiter.getActiveMotions()) {
        if (m.piece_id == 2) {
            found = true;
            CHECK(m.to == Position{0, 3}); // לא קוצר בכלל
        }
    }
    CHECK(found);
}
