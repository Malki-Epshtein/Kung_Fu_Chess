#include "../doctest.h"
#include "../engine/ScoreObserver.h"

namespace {
    class TestPiece : public Piece {
    public:
        TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
            : Piece(id, color, kind, cell, Chess::State::Idle) {}
        std::vector<Position> getValidMoves() const override { return {}; }
    };
}

TEST_CASE("ScoreObserver - מתחיל עם ניקוד אפס לשני הצדדים") {
    ScoreObserver score;
    CHECK(score.getScore(Chess::Color::White) == 0);
    CHECK(score.getScore(Chess::Color::Black) == 0);
}

TEST_CASE("ScoreObserver - אכילת חייל שחור מוסיפה 1 נקודה ללבן") {
    ScoreObserver score;
    TestPiece blackPawn(1, Chess::Color::Black, Chess::Kind::Pawn, {0, 0});

    score.onPieceCaptured(blackPawn);

    CHECK(score.getScore(Chess::Color::White) == 1);
    CHECK(score.getScore(Chess::Color::Black) == 0);
}

TEST_CASE("ScoreObserver - אכילת מלכה לבנה מוסיפה 9 נקודות לשחור") {
    ScoreObserver score;
    TestPiece whiteQueen(2, Chess::Color::White, Chess::Kind::Queen, {0, 0});

    score.onPieceCaptured(whiteQueen);

    CHECK(score.getScore(Chess::Color::Black) == 9);
    CHECK(score.getScore(Chess::Color::White) == 0);
}

TEST_CASE("ScoreObserver - כל ערכי הכלים הסטנדרטיים נכונים") {
    ScoreObserver score;
    TestPiece knight(1, Chess::Color::Black, Chess::Kind::Knight, {0, 0});
    TestPiece bishop(2, Chess::Color::Black, Chess::Kind::Bishop, {0, 1});
    TestPiece rook(3, Chess::Color::Black, Chess::Kind::Rook, {0, 2});

    score.onPieceCaptured(knight);
    score.onPieceCaptured(bishop);
    score.onPieceCaptured(rook);

    CHECK(score.getScore(Chess::Color::White) == 3 + 3 + 5);
}

TEST_CASE("ScoreObserver - אכילת מלך לא מוסיפה ניקוד (המשחק נגמר בדרך אחרת)") {
    ScoreObserver score;
    TestPiece king(1, Chess::Color::Black, Chess::Kind::King, {0, 0});

    score.onPieceCaptured(king);

    CHECK(score.getScore(Chess::Color::White) == 0);
}

TEST_CASE("ScoreObserver - ניקוד מצטבר על פני כמה אכילות של אותו צבע") {
    ScoreObserver score;
    TestPiece p1(1, Chess::Color::Black, Chess::Kind::Pawn, {0, 0});
    TestPiece p2(2, Chess::Color::Black, Chess::Kind::Pawn, {0, 1});

    score.onPieceCaptured(p1);
    score.onPieceCaptured(p2);

    CHECK(score.getScore(Chess::Color::White) == 2);
}
