#include "../doctest.h"
#include "../engine/MoveLogObserver.h"

namespace {
    class TestPiece : public Piece {
    public:
        TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
            : Piece(id, color, kind, cell, Chess::State::Idle) {}
        std::vector<Position> getValidMoves() const override { return {}; }
    };
}

TEST_CASE("MoveLogObserver - מתחיל עם רשימות ריקות לשני הצדדים") {
    MoveLogObserver log;
    CHECK(log.getMoves(Chess::Color::White).empty());
    CHECK(log.getMoves(Chess::Color::Black).empty());
}

TEST_CASE("MoveLogObserver - מהלך לבן נכנס לרשימת הלבן בלבד") {
    MoveLogObserver log;
    TestPiece whiteRook(1, Chess::Color::White, Chess::Kind::Rook, {3, 3});

    log.onMoveCompleted(whiteRook, {3, 3}, {3, 6});

    REQUIRE(log.getMoves(Chess::Color::White).size() == 1);
    CHECK(log.getMoves(Chess::Color::Black).empty());
}

TEST_CASE("MoveLogObserver - תיאור המהלך כולל צבע, סוג כלי, ומיקומי מקור/יעד") {
    MoveLogObserver log;
    TestPiece whiteRook(1, Chess::Color::White, Chess::Kind::Rook, {3, 3});

    log.onMoveCompleted(whiteRook, {3, 3}, {3, 6});

    std::string entry = log.getMoves(Chess::Color::White)[0];
    CHECK(entry.find("White") != std::string::npos);
    CHECK(entry.find("Rook") != std::string::npos);
    CHECK(entry.find("(3,3)") != std::string::npos);
    CHECK(entry.find("(3,6)") != std::string::npos);
}

TEST_CASE("MoveLogObserver - מהלכים של שני הצדדים נשמרים ברשימות נפרדות") {
    MoveLogObserver log;
    TestPiece whitePiece(1, Chess::Color::White, Chess::Kind::Pawn, {6, 0});
    TestPiece blackPiece(2, Chess::Color::Black, Chess::Kind::Pawn, {1, 0});

    log.onMoveCompleted(whitePiece, {6, 0}, {5, 0});
    log.onMoveCompleted(blackPiece, {1, 0}, {2, 0});

    CHECK(log.getMoves(Chess::Color::White).size() == 1);
    CHECK(log.getMoves(Chess::Color::Black).size() == 1);
}

TEST_CASE("MoveLogObserver - כמה מהלכים של אותו צבע נשמרים בסדר") {
    MoveLogObserver log;
    TestPiece whitePiece(1, Chess::Color::White, Chess::Kind::Pawn, {6, 0});

    log.onMoveCompleted(whitePiece, {6, 0}, {5, 0});
    log.onMoveCompleted(whitePiece, {5, 0}, {4, 0});

    REQUIRE(log.getMoves(Chess::Color::White).size() == 2);
    CHECK(log.getMoves(Chess::Color::White)[0].find("(6,0)") != std::string::npos);
    CHECK(log.getMoves(Chess::Color::White)[1].find("(5,0)") != std::string::npos);
}
