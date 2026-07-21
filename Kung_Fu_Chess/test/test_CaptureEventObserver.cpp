#include "../doctest.h"
#include "../server/engine/CaptureEventObserver.h"

namespace {
    class TestPiece : public Piece {
    public:
        TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
            : Piece(id, color, kind, cell, Chess::State::Idle) {}
        std::vector<Position> getValidMoves() const override { return {}; }
    };
}

TEST_CASE("CaptureEventObserver - בלי onNewCapture מוגדר, onPieceCaptured לא קורס") {
    CaptureEventObserver observer;
    TestPiece pawn(1, Chess::Color::Black, Chess::Kind::Pawn, {3, 4});

    observer.onPieceCaptured(pawn);
    // No CHECK needed - the test passes simply by not crashing.
}

TEST_CASE("CaptureEventObserver - onNewCapture נקרא עם הסוג, הצבע והמשבצת הנכונים") {
    CaptureEventObserver observer;
    TestPiece knight(1, Chess::Color::White, Chess::Kind::Knight, {2, 5});

    bool called = false;
    CaptureEvent received{};
    observer.onNewCapture = [&](const CaptureEvent& event) {
        called = true;
        received = event;
    };

    observer.onPieceCaptured(knight);

    REQUIRE(called);
    CHECK(received.kind == Chess::Kind::Knight);
    CHECK(received.color == Chess::Color::White);
    CHECK(received.cell == Position{2, 5});
}

TEST_CASE("CaptureEventObserver - כמה אכילות מפעילות את ה-callback כל פעם מחדש") {
    CaptureEventObserver observer;
    TestPiece a(1, Chess::Color::Black, Chess::Kind::Pawn, {0, 0});
    TestPiece b(2, Chess::Color::White, Chess::Kind::Rook, {7, 7});

    int callCount = 0;
    observer.onNewCapture = [&](const CaptureEvent&) { ++callCount; };

    observer.onPieceCaptured(a);
    observer.onPieceCaptured(b);

    CHECK(callCount == 2);
}
