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
    MoveLogObserver log(8);
    CHECK(log.getMoves(Chess::Color::White).empty());
    CHECK(log.getMoves(Chess::Color::Black).empty());
}

TEST_CASE("MoveLogObserver - מהלך לבן נכנס לרשימת הלבן בלבד") {
    MoveLogObserver log(8);
    TestPiece whiteRook(1, Chess::Color::White, Chess::Kind::Rook, {3, 3});

    log.onMoveCompleted(whiteRook, {3, 3}, {3, 6}, false);

    REQUIRE(log.getMoves(Chess::Color::White).size() == 1);
    CHECK(log.getMoves(Chess::Color::Black).empty());
}

TEST_CASE("MoveLogObserver - תא היעד ממופה נכון ל-file+rank לפי גובה הלוח") {
    MoveLogObserver log(8);
    TestPiece whiteRook(1, Chess::Color::White, Chess::Kind::Rook, {3, 3});
    // row=3, col=6 -> file 'g' (a+6), rank = 8-3 = 5
    log.onMoveCompleted(whiteRook, {3, 3}, {3, 6}, false);
    CHECK(log.getMoves(Chess::Color::White)[0].notation == "Rg5");
}

TEST_CASE("MoveLogObserver - רגלי לא מקבל אות כלי") {
    MoveLogObserver log(8);
    TestPiece whitePawn(1, Chess::Color::White, Chess::Kind::Pawn, {6, 0});
    // row=5, col=0 -> "a", rank = 8-5 = 3
    log.onMoveCompleted(whitePawn, {6, 0}, {5, 0}, false);
    CHECK(log.getMoves(Chess::Color::White)[0].notation == "a3");
}

TEST_CASE("MoveLogObserver - אכילה מוסיפה x לפני תא היעד") {
    MoveLogObserver log(8);
    TestPiece whiteKnight(1, Chess::Color::White, Chess::Kind::Knight, {3, 3});
    log.onMoveCompleted(whiteKnight, {3, 3}, {1, 4}, true);
    CHECK(log.getMoves(Chess::Color::White)[0].notation == "Nxe7");
}

TEST_CASE("MoveLogObserver - אכילת רגלי מוסיפה את אות טור המקור לפני ה-x") {
    MoveLogObserver log(8);
    TestPiece whitePawn(1, Chess::Color::White, Chess::Kind::Pawn, {4, 4});
    // capture diagonally: from col=4 ('e') to col=5, row=3 -> rank 5
    log.onMoveCompleted(whitePawn, {4, 4}, {3, 5}, true);
    CHECK(log.getMoves(Chess::Color::White)[0].notation == "exf5");
}

TEST_CASE("MoveLogObserver - מהלכים של שני הצדדים נשמרים ברשימות נפרדות") {
    MoveLogObserver log(8);
    TestPiece whitePiece(1, Chess::Color::White, Chess::Kind::Pawn, {6, 0});
    TestPiece blackPiece(2, Chess::Color::Black, Chess::Kind::Pawn, {1, 0});

    log.onMoveCompleted(whitePiece, {6, 0}, {5, 0}, false);
    log.onMoveCompleted(blackPiece, {1, 0}, {2, 0}, false);

    CHECK(log.getMoves(Chess::Color::White).size() == 1);
    CHECK(log.getMoves(Chess::Color::Black).size() == 1);
}

TEST_CASE("MoveLogObserver - כמה מהלכים של אותו צבע נשמרים בסדר") {
    MoveLogObserver log(8);
    TestPiece whitePiece(1, Chess::Color::White, Chess::Kind::Pawn, {6, 0});

    log.onMoveCompleted(whitePiece, {6, 0}, {5, 0}, false);
    log.onMoveCompleted(whitePiece, {5, 0}, {4, 0}, false);

    REQUIRE(log.getMoves(Chess::Color::White).size() == 2);
    CHECK(log.getMoves(Chess::Color::White)[0].notation == "a3");
    CHECK(log.getMoves(Chess::Color::White)[1].notation == "a4");
}

TEST_CASE("MoveLogObserver - לכל רשומה יש חותמת זמן בפורמט MM:SS.mmm") {
    MoveLogObserver log(8);
    TestPiece whitePiece(1, Chess::Color::White, Chess::Kind::Pawn, {6, 0});

    log.onMoveCompleted(whitePiece, {6, 0}, {5, 0}, false);

    const std::string& ts = log.getMoves(Chess::Color::White)[0].timestamp;
    REQUIRE(ts.size() == 9); // "00:00.012" וכו'
    CHECK(ts[2] == ':');
    CHECK(ts[5] == '.');
}
