#include "../doctest.h"
#include "../rules/rule_engine.h"
#include "../model/Board.h"
#include <memory>

class TestPiece : public Piece {
public:
    TestPiece(Chess::Color c, Chess::Kind k, Position pos)
        : Piece(1, c, k, pos, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

static std::shared_ptr<Piece> make(Chess::Color c, Chess::Kind k, Position pos) {
    return std::make_shared<TestPiece>(c, k, pos);
}

// ==================== outside_board ====================

TEST_CASE("outside_board - from מחוץ לגבולות") {
    Board board(8, 8);
    auto result = RuleEngine::validateMove(board, {-1, 0}, {3, 3});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "outside_board");
}

TEST_CASE("outside_board - to מחוץ לגבולות") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {8, 3});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "outside_board");
}

TEST_CASE("outside_board - שניהם מחוץ לגבולות") {
    Board board(8, 8);
    auto result = RuleEngine::validateMove(board, {-1, -1}, {9, 9});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "outside_board");
}

// ==================== empty_source ====================

TEST_CASE("empty_source - תא מקור ריק") {
    Board board(8, 8);
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 4});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "empty_source");
}

TEST_CASE("empty_source - EmptyPiece בתא המקור") {
    Board board(8, 8);
    // הלוח מאותחל עם EmptyPiece בכל תא
    auto result = RuleEngine::validateMove(board, {0, 0}, {0, 1});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "empty_source");
}

// ==================== friendly_destination ====================

TEST_CASE("friendly_destination - לבן מנסה לדרוס לבן") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook,  {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn,  {3, 4}), {3, 4});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 4});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "friendly_destination");
}

TEST_CASE("friendly_destination - שחור מנסה לדרוס שחור") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::Black, Chess::Kind::Queen, {4, 4}), {4, 4});
    board.addPiece(make(Chess::Color::Black, Chess::Kind::Pawn,  {4, 5}), {4, 5});
    auto result = RuleEngine::validateMove(board, {4, 4}, {4, 5});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "friendly_destination");
}

// ==================== illegal_piece_move ====================

TEST_CASE("illegal_piece_move - צריח מנסה לזוז באלכסון") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {5, 5});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("illegal_piece_move - רץ מנסה לזוז ישר") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Bishop, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 5});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("illegal_piece_move - רגלי לבן מנסה לזוז אחורה") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    auto result = RuleEngine::validateMove(board, {4, 3}, {5, 3});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("illegal_piece_move - רגלי מנסה לאכול ישר קדימה") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn,  {4, 3}), {4, 3});
    board.addPiece(make(Chess::Color::Black, Chess::Kind::Pawn,  {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {4, 3}, {3, 3});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("illegal_piece_move - מלך מנסה לזוז שתי משבצות") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::King, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 5});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("illegal_piece_move - צריח חסום על ידי ידיד, מנסה לעבור מעבר לו") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 5}), {3, 5});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 6});
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}

// ==================== ok - מהלכים חוקיים ====================

TEST_CASE("ok - צריח זז ישר") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 6});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - רץ זז באלכסון") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Bishop, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {5, 5});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - פרש קופץ מעל כלים") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Knight, {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn,   {3, 4}), {3, 4});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn,   {4, 3}), {4, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {1, 2});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - צריח אוכל אויב") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::Black, Chess::Kind::Pawn, {3, 6}), {3, 6});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 6});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - רגלי לבן צועד קדימה") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    auto result = RuleEngine::validateMove(board, {4, 3}, {3, 3});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - רגלי לבן אוכל באלכסון") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {4, 3}), {4, 3});
    board.addPiece(make(Chess::Color::Black, Chess::Kind::Pawn, {3, 4}), {3, 4});
    auto result = RuleEngine::validateMove(board, {4, 3}, {3, 4});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - מלך זז משבצת אחת") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::King, {3, 3}), {3, 3});
    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 4});
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("ok - מלכה זזה ישר ואלכסון") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Queen, {3, 3}), {3, 3});
    CHECK(RuleEngine::validateMove(board, {3, 3}, {3, 7}).is_valid);
    CHECK(RuleEngine::validateMove(board, {3, 3}, {7, 7}).is_valid);
}

// ==================== relaxedBlocking (סעיף 7) ====================

TEST_CASE("relaxedBlocking=true - צריח חסום עי ידיד יכול לזוז מעבר לו") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 5}), {3, 5});

    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 6}, /*relaxedBlocking=*/true);
    CHECK(result.is_valid);
    CHECK(result.reason == "ok");
}

TEST_CASE("relaxedBlocking=true - friendly_destination עדיין נדחה כרגיל (לא מתרכך)") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 4}), {3, 4});

    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 4}, /*relaxedBlocking=*/true);
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "friendly_destination");
}

TEST_CASE("relaxedBlocking=false (ברירת מחדל) - צריח חסום עי ידיד עדיין נדחה כמו קודם") {
    Board board(8, 8);
    board.addPiece(make(Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.addPiece(make(Chess::Color::White, Chess::Kind::Pawn, {3, 5}), {3, 5});

    auto result = RuleEngine::validateMove(board, {3, 3}, {3, 6}); // בלי הפרמטר בכלל
    CHECK_FALSE(result.is_valid);
    CHECK(result.reason == "illegal_piece_move");
}
