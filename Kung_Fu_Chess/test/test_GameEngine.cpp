#include "../doctest.h"
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

static const SnapshotPiece* findAt(const GameSnapshot& snap, Position pos) {
    for (const auto& p : snap.pieces)
        if (p.cell == pos)
            return &p;
    return nullptr;
}

// ==================== game_over ====================

TEST_CASE("requestMove - דוחה עם game_over אחרי אכילת מלך") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {0, 1}), {0, 1});
    GameEngine engine(board);

    auto move = engine.requestMove({0, 0}, {0, 1});
    CHECK(move.is_accepted);
    CHECK(move.reason == "ok");

    engine.wait(1000);
    CHECK(engine.isGameOver());

    auto after = engine.requestMove({0, 1}, {0, 2});
    CHECK_FALSE(after.is_accepted);
    CHECK(after.reason == "game_over");

    // הלוח לא השתנה בעקבות המהלך הנדחה
    CHECK(findAt(engine.snapshot(), {0, 1}) != nullptr);
    CHECK(findAt(engine.snapshot(), {0, 2}) == nullptr);
}

// ==================== motion_in_progress ====================

TEST_CASE("requestMove - דוחה כאשר תנועה כבר פעילה מאותו כלי") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    auto first = engine.requestMove({3, 3}, {3, 6});
    CHECK(first.is_accepted);

    auto second = engine.requestMove({3, 3}, {3, 5});
    CHECK_FALSE(second.is_accepted);
    CHECK(second.reason == "motion_in_progress");
}

TEST_CASE("requestMove - דוחה מהלך שיעדו כבר יעד של תנועה פעילה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {5, 6}), {5, 6});
    GameEngine engine(board);

    auto first = engine.requestMove({0, 0}, {0, 6});
    CHECK(first.is_accepted);

    auto second = engine.requestMove({5, 6}, {0, 6});
    CHECK_FALSE(second.is_accepted);
    CHECK(second.reason == "motion_in_progress");
}

// ==================== simultaneousMode ====================

TEST_CASE("requestMove - כשsimultaneousMode כבוי (ברירת מחדל), התנהגות motion_in_progress נשארת בדיוק כמו קודם (לפי from/to חופפים)") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {5, 5}), {5, 5});
    GameEngine engine(board); // ברירת מחדל: simultaneousMode=false

    auto first = engine.requestMove({0, 0}, {0, 6});
    CHECK(first.is_accepted);

    // כלי אחר לגמרי, בלי from/to חופפים - זו התנהגות קיימת שלא השתנתה
    auto second = engine.requestMove({5, 5}, {5, 6});
    CHECK(second.is_accepted);
}

TEST_CASE("requestMove - כשsimultaneousMode כבוי (ברירת מחדל), אין מושג של צינון - אפשר לצוות על אותו כלי מיד אחרי הגעה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board); // ברירת מחדל: simultaneousMode=false

    auto first = engine.requestMove({3, 3}, {3, 4});
    CHECK(first.is_accepted);

    engine.wait(1000); // הגעה

    auto second = engine.requestMove({3, 4}, {3, 5}); // מיד אחרי ההגעה, בלי המתנת צינון
    CHECK(second.is_accepted);
}

TEST_CASE("requestMove - כשsimultaneousMode דלוק, שני כלים שונים יכולים לזוז בו-זמנית") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {5, 5}), {5, 5});
    GameEngine engine(board, /*simultaneousMode=*/true);

    auto first = engine.requestMove({0, 0}, {0, 6});
    CHECK(first.is_accepted);

    auto second = engine.requestMove({5, 5}, {5, 6});
    CHECK(second.is_accepted);
}

TEST_CASE("requestMove - כשsimultaneousMode דלוק, אותו כלי נדחה בזמן שהוא בתנועה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    auto first = engine.requestMove({3, 3}, {3, 6});
    CHECK(first.is_accepted);

    auto second = engine.requestMove({3, 3}, {3, 5});
    CHECK_FALSE(second.is_accepted);
    CHECK(second.reason == "motion_in_progress");
}

TEST_CASE("requestMove - כשsimultaneousMode דלוק, אותו כלי בזמן צינון אחרי הגעה - הבקשה מתווספת לתור premove") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    auto first = engine.requestMove({3, 3}, {3, 4});
    CHECK(first.is_accepted);

    engine.wait(1000); // הגעה, מתחיל צינון

    auto second = engine.requestMove({3, 4}, {3, 5});
    CHECK(second.is_accepted);
    CHECK(second.reason == "queued");

    // התור לא יורה מיד - הכלי עדיין ב-{3,4} כי הצינון עוד לא נגמר
    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr);
}

TEST_CASE("requestMove - כשsimultaneousMode דלוק, אותו כלי בתנועה פעילה (לא צינון) נדחה לגמרי") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    auto first = engine.requestMove({3, 3}, {3, 6});
    CHECK(first.is_accepted);

    auto second = engine.requestMove({3, 3}, {3, 5}); // אותו כלי, עדיין בתנועה פעילה
    CHECK_FALSE(second.is_accepted);
    CHECK(second.reason == "motion_in_progress");
}

// ==================== premove ====================

TEST_CASE("requestMove - premove יורה אוטומטית ברגע שהצינון נגמר") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    engine.requestMove({3, 3}, {3, 4});
    engine.wait(1000); // הגעה, מתחיל צינון

    auto queued = engine.requestMove({3, 4}, {3, 5});
    CHECK(queued.reason == "queued");
    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr); // עדיין לא ירה

    engine.wait(999);
    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr); // עדיין בצינון

    engine.wait(1); // הצינון נגמר בדיוק עכשיו - ה-premove אמור לירות ולהתחיל תנועה חדשה
    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr); // כבר יורה, אבל עדיין לא הגיע ל-(3,5)

    engine.wait(1000); // התנועה שנורתה על ידי premove מסתיימת
    CHECK(findAt(engine.snapshot(), {3, 4}) == nullptr);
    CHECK(findAt(engine.snapshot(), {3, 5}) != nullptr);
}

TEST_CASE("requestMove - מהלך לא-חוקי מבטל premove ממתין") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    engine.requestMove({3, 3}, {3, 4});
    engine.wait(1000); // הגעה, מתחיל צינון

    auto queued = engine.requestMove({3, 4}, {3, 6}); // premove לגיטימי בזמן צינון
    CHECK(queued.reason == "queued");

    auto illegal = engine.requestMove({3, 4}, {4, 3}); // אלכסון - לא צורת מהלך חוקית לצריח, מבטל את הpremove
    CHECK_FALSE(illegal.is_accepted);
    CHECK(illegal.reason == "illegal_piece_move");

    engine.wait(2000); // מספיק זמן לצינון ולירי premove, אם הוא לא היה מבוטל
    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr); // נשאר במקומו - הpremove בוטל
}

TEST_CASE("requestMove - premove שהיעד שלו הפך ללא-חוקי עד שהצינון נגמר נמחק בשקט בלי לירות") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    engine.requestMove({3, 3}, {3, 4});
    engine.wait(1000); // הגעה, מתחיל צינון

    auto queued = engine.requestMove({3, 4}, {3, 6}); // premove לגיטימי כרגע
    CHECK(queued.reason == "queued");

    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Bishop, {3, 6}), {3, 6}); // היעד הופך ידידותי-תפוס

    engine.wait(1000); // הצינון נגמר - premove מנסה לירות, אבל היעד כבר לא חוקי

    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr); // נשאר במקומו - premove נמחק בשקט
    CHECK_FALSE(engine.isGameOver());
}

// ==================== relaxedBlocking - מהלך דרך חוסם (סעיף 7) ====================

TEST_CASE("requestMove - כשsimultaneousMode דלוק, מהלך צריח דרך חוסם ידידותי מתקבל") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {3, 5}), {3, 5});
    GameEngine engine(board, /*simultaneousMode=*/true);

    auto result = engine.requestMove({3, 3}, {3, 6});
    CHECK(result.is_accepted);
    CHECK(result.reason == "ok");
}

TEST_CASE("requestMove - כשsimultaneousMode כבוי (ברירת מחדל), מהלך דרך חוסם עדיין נדחה כרגיל") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {3, 5}), {3, 5});
    GameEngine engine(board); // ברירת מחדל

    auto result = engine.requestMove({3, 3}, {3, 6});
    CHECK_FALSE(result.is_accepted);
    CHECK(result.reason == "illegal_piece_move");
}

// ==================== סיבות לא-חוקיות מ-RuleEngine ====================

TEST_CASE("requestMove - מעביר הלאה סיבות לא-חוקיות בלי להתחיל תנועה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    auto outside = engine.requestMove({3, 3}, {8, 3});
    CHECK_FALSE(outside.is_accepted);
    CHECK(outside.reason == "outside_board");

    auto empty = engine.requestMove({0, 0}, {0, 1});
    CHECK_FALSE(empty.is_accepted);
    CHECK(empty.reason == "empty_source");

    auto illegal = engine.requestMove({3, 3}, {5, 5});
    CHECK_FALSE(illegal.is_accepted);
    CHECK(illegal.reason == "illegal_piece_move");

    // אף אחד מהמהלכים הדחויים לא התחיל תנועה - הכלי עדיין במקורו אחרי זמן רב
    engine.wait(5000);
    CHECK(findAt(engine.snapshot(), {3, 3}) != nullptr);
}

TEST_CASE("requestMove - friendly_destination לא מתחיל תנועה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {3, 4}), {3, 4});
    GameEngine engine(board);

    auto result = engine.requestMove({3, 3}, {3, 4});
    CHECK_FALSE(result.is_accepted);
    CHECK(result.reason == "friendly_destination");
}

// ==================== ok - מתחיל תנועה, הלוח לא משתנה מיד ====================

TEST_CASE("requestMove - מהלך חוקי מחזיר ok אך לא מזיז את הכלי מיידית") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    auto result = engine.requestMove({3, 3}, {3, 6});
    CHECK(result.is_accepted);
    CHECK(result.reason == "ok");

    CHECK(findAt(engine.snapshot(), {3, 3}) != nullptr);
    CHECK(findAt(engine.snapshot(), {3, 6}) == nullptr);
}

// ==================== תזמון תנועה (wait) ====================

TEST_CASE("wait - תנועה של משבצת אחת מגיעה בדיוק אחרי 1000 מ\"ש") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    engine.requestMove({3, 3}, {3, 4});

    engine.wait(999);
    CHECK(findAt(engine.snapshot(), {3, 3}) != nullptr);
    CHECK(findAt(engine.snapshot(), {3, 4}) == nullptr);

    engine.wait(1);
    CHECK(findAt(engine.snapshot(), {3, 3}) == nullptr);
    CHECK(findAt(engine.snapshot(), {3, 4}) != nullptr);
}

TEST_CASE("wait - תנועה של שתי משבצות לוקחת 2000 מ\"ש") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    engine.requestMove({3, 3}, {3, 5});

    engine.wait(1000);
    CHECK(findAt(engine.snapshot(), {3, 3}) != nullptr);

    engine.wait(1000);
    CHECK(findAt(engine.snapshot(), {3, 3}) == nullptr);
    CHECK(findAt(engine.snapshot(), {3, 5}) != nullptr);
}

TEST_CASE("wait - תנועה אלכסונית נמדדת לפי צעדי-תא ולא לפי מרחק אווירי") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Bishop, {0, 0}), {0, 0});
    GameEngine engine(board);
    engine.requestMove({0, 0}, {3, 3}); // שלוש משבצות אלכסוניות = 3000 מ"ש

    engine.wait(2999);
    CHECK(findAt(engine.snapshot(), {0, 0}) != nullptr);

    engine.wait(1);
    CHECK(findAt(engine.snapshot(), {3, 3}) != nullptr);
}

// ==================== הגעה ואכילה ====================

TEST_CASE("הגעה - אכילת כלי שאינו מלך מסירה אותו ולא מסיימת את המשחק") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {3, 6}), {3, 6});
    GameEngine engine(board);

    engine.requestMove({3, 3}, {3, 6});
    engine.wait(3000);

    auto snap = engine.snapshot();
    auto* mover = findAt(snap, {3, 6});
    CHECK(mover != nullptr);
    CHECK(mover->color == Chess::Color::White);
    CHECK(mover->kind == Chess::Kind::Rook);
    CHECK(findAt(engine.snapshot(), {3, 3}) == nullptr);
    CHECK_FALSE(engine.isGameOver());
}

TEST_CASE("הגעה - אכילת מלך מסיימת את המשחק") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {3, 6}), {3, 6});
    GameEngine engine(board);

    CHECK_FALSE(engine.isGameOver());
    engine.requestMove({3, 3}, {3, 6});
    engine.wait(3000);

    CHECK(engine.isGameOver());
    CHECK(engine.snapshot().game_over);
}

// ==================== requestJump ====================

TEST_CASE("requestJump - כלי אחר שמחליק ליעד זהה לתא הקפיצה לא חוסם את הקפיצה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {4, 6}), {4, 6});
    GameEngine engine(board, /*simultaneousMode=*/true);

    engine.requestMove({4, 6}, {4, 3}); // הצריח מחליק ליעד שהוא בדיוק תא החייל

    auto jumped = engine.requestJump({4, 3});
    CHECK(jumped.is_accepted);
    CHECK(jumped.reason == "ok");
}

TEST_CASE("requestJump - כלי שכבר בתנועה בעצמו נדחה עם motion_in_progress") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    GameEngine engine(board, /*simultaneousMode=*/true);

    engine.requestMove({4, 3}, {3, 3});

    auto jumped = engine.requestJump({4, 3});
    CHECK_FALSE(jumped.is_accepted);
    CHECK(jumped.reason == "motion_in_progress");
}

// ==================== legalDestinationsFrom ====================

TEST_CASE("legalDestinationsFrom - מחזיר את יעדי הכלל של הכלי, ישירות מ-PieceRules") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);

    auto moves = engine.legalDestinationsFrom({3, 3});

    bool found = false;
    for (const auto& pos : moves)
        if (pos == Position{3, 6}) found = true;
    CHECK(found);
    CHECK(moves.size() == 14); // צריח מהמרכז בלוח ריק
}

TEST_CASE("legalDestinationsFrom - תא ריק מחזיר רשימה ריקה") {
    auto board = std::make_shared<Board>(8, 8);
    GameEngine engine(board);

    auto moves = engine.legalDestinationsFrom({3, 3});
    CHECK(moves.empty());
}

// ==================== snapshot ====================

TEST_CASE("snapshot - חושף רוחב, גובה, כלים, ודגל game_over") {
    auto board = std::make_shared<Board>(5, 4);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::King, {1, 1}), {1, 1});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {2, 2}), {2, 2});
    GameEngine engine(board);

    auto snap = engine.snapshot();
    CHECK(snap.board_width == 5);
    CHECK(snap.board_height == 4);
    CHECK_FALSE(snap.game_over);
    CHECK(snap.pieces.size() == 2);

    auto* king = findAt(snap, {1, 1});
    CHECK(king != nullptr);
    CHECK(king->kind == Chess::Kind::King);
    CHECK(king->color == Chess::Color::White);
}
