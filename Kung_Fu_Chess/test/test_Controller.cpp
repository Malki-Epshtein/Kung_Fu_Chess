#include "../doctest.h"
#include "../input/Controller.h"
#include "../model/Board.h"
#include "../view/ViewConfig.h"
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

// לחיצה על מרכז התא (row, col) בפיקסלים, לפי ViewConfig::CELL_SIZE
static void click(Controller& controller, int row, int col) {
    int x = col * ViewConfig::CELL_SIZE + 50;
    int y = row * ViewConfig::CELL_SIZE + 50;
    controller.handleMouseClick(x, y);
}

// ==================== קליק ראשון - בחירה ====================

TEST_CASE("click - קליק ראשון על כלי ואז קליק שני על יעד חוקי שולח מהלך") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3); // בחירת הצריח
    click(controller, 3, 6); // יעד חוקי וריק

    engine.wait(3000);
    CHECK(findAt(controller.getSnapshot(), {3, 3}) == nullptr);
    CHECK(findAt(controller.getSnapshot(), {3, 6}) != nullptr);
}

TEST_CASE("click - קליק ראשון על תא ריק לא בוחר כלום") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 0, 0); // תא ריק - לא אמור לבחור כלום

    click(controller, 3, 3); // קליק ראשון אמיתי - בוחר את הצריח
    click(controller, 3, 6); // קליק שני - שולח מהלך

    engine.wait(3000);
    CHECK(findAt(controller.getSnapshot(), {3, 6}) != nullptr);
}

// ==================== קליק כפול על אותו כלי - קפיצה ====================

TEST_CASE("click - קליק שני על אותו כלי נבחר שולח קפיצה, לא מהלך") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3); // בחירת הצריח
    click(controller, 3, 3); // קליק שני על אותו תא - קפיצה, לא בחירה מחדש/מהלך

    CHECK(findAt(controller.getSnapshot(), {3, 3})->state == Chess::State::Jump);
    CHECK_FALSE(controller.getSnapshot().has_selection); // הבחירה התאפסה אחרי הקפיצה
}

// ==================== קליק מחוץ ללוח ====================

TEST_CASE("click - קליק מחוץ ללוח ללא בחירה מתעלם") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    controller.handleMouseClick(-10, -10); // מחוץ ללוח, אין בחירה קודמת

    click(controller, 3, 3); // קליק ראשון אמיתי - בוחר את הצריח
    click(controller, 3, 6); // קליק שני - שולח מהלך

    engine.wait(3000);
    CHECK(findAt(controller.getSnapshot(), {3, 6}) != nullptr);
}

TEST_CASE("click - קליק מחוץ ללוח עם כלי נבחר מבטל את הבחירה ולא שולח מהלך") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3);              // בחירת הצריח
    controller.handleMouseClick(-10, -10); // מחוץ ללוח - מבטל בחירה, לא שולח מהלך

    // אם הבחירה לא בוטלה, קליק על תא ריק היה מתפרש כ"קליק שני" ושולח מהלך
    click(controller, 3, 6); // תא ריק, ללא כלי - אמור לא לעשות כלום

    engine.wait(5000);
    CHECK(findAt(controller.getSnapshot(), {3, 3}) != nullptr);
    CHECK(findAt(controller.getSnapshot(), {3, 6}) == nullptr);
}

// ==================== ניקוי בחירה אחרי קליק שני - גם במהלך לא-חוקי ====================

TEST_CASE("click - קליק שני על יעד לא-חוקי מנקה את הבחירה בכל זאת") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3}); // ידיד, לא חוסם שורה 3
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3); // בחירת הצריח
    click(controller, 4, 3); // יעד ידידותי - נדחה על ידי GameEngine, אך הבחירה מתנקה

    // אם הבחירה לא התנקתה, הקליק הבא (תא ריק) היה מתפרש כ"קליק שני" ושולח מהלך חוקי
    click(controller, 3, 6); // תא ריק, ללא כלי - אמור לא לעשות כלום

    engine.wait(5000);
    CHECK(findAt(controller.getSnapshot(), {3, 3}) != nullptr);
    CHECK(findAt(controller.getSnapshot(), {3, 6}) == nullptr);
}

// ==================== קליק שני על כלי מאותו צבע - מחליף בחירה ====================

TEST_CASE("click - קליק שני על כלי מאותו צבע מחליף את הבחירה במקום לשלוח מהלך") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook,   {0, 0}), {0, 0});
    board->addPiece(make(2, Chess::Color::White, Chess::Kind::Bishop, {0, 2}), {0, 2});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 0, 0); // בחירת הצריח
    click(controller, 0, 2); // קליק על הרץ - מחליף בחירה, לא שולח מהלך
    click(controller, 2, 4); // יעד חוקי לרץ (אלכסון), לא חוקי לצריח

    engine.wait(2000);

    // הרץ זז, הצריח נשאר במקומו - מוכיח שהבחירה עברה אל הרץ
    CHECK(findAt(controller.getSnapshot(), {0, 0}) != nullptr);
    CHECK(findAt(controller.getSnapshot(), {0, 2}) == nullptr);
    CHECK(findAt(controller.getSnapshot(), {2, 4}) != nullptr);
}

// ==================== קליק שני על כלי אויב - שולח מהלך אכילה רגיל ====================

TEST_CASE("click - קליק שני על כלי אויב שולח בקשת אכילה רגילה") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board->addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {3, 6}), {3, 6});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3); // בחירת הצריח
    click(controller, 3, 6); // כלי אויב - קליק שני רגיל, שולח מהלך אכילה

    engine.wait(3000);

    auto snap = controller.getSnapshot();
    auto* mover = findAt(snap, {3, 6});
    CHECK(mover != nullptr);
    CHECK(mover->color == Chess::Color::White);
    CHECK(findAt(controller.getSnapshot(), {3, 3}) == nullptr);
}

// ==================== handleWait ====================

TEST_CASE("handleWait - מאציל את קידום הזמן ל-GameEngine") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3);
    click(controller, 3, 4);

    controller.handleWait(999);
    CHECK(findAt(controller.getSnapshot(), {3, 3}) != nullptr);

    controller.handleWait(1);
    CHECK(findAt(controller.getSnapshot(), {3, 4}) != nullptr);
}

// ==================== getSnapshot - has_selection / selected_cell ====================

TEST_CASE("getSnapshot - אין בחירה לפני קליק ראשון") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    CHECK_FALSE(controller.getSnapshot().has_selection);
}

TEST_CASE("getSnapshot - קליק ראשון על כלי חושף אותו כתא נבחר") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3);

    auto snap = controller.getSnapshot();
    CHECK(snap.has_selection);
    CHECK(snap.selected_cell == Position{ 3, 3 });
}

TEST_CASE("getSnapshot - קליק שני מנקה את הבחירה מתמונת המצב") {
    auto board = std::make_shared<Board>(8, 8);
    board->addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    GameEngine engine(board);
    Controller controller(engine);

    click(controller, 3, 3);
    click(controller, 3, 6);

    CHECK_FALSE(controller.getSnapshot().has_selection);
}
