#include "../doctest.h"
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

    auto* mover = findAt(engine.snapshot(), {3, 6});
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
