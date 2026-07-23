#include "../doctest.h"
#include "../shared/protocol/CaptureEventCodec.h"

TEST_CASE("CaptureEventCodec - encode/decode משמרים סוג, צבע ומשבצת") {
    CaptureEvent original{ Chess::Kind::Queen, Chess::Color::Black, Position{4, 3}, 12, 7, Position{4, 3}, 0.63 };

    CaptureEvent decoded = CaptureEventCodec::decode(CaptureEventCodec::encode(original));

    CHECK(decoded.kind == Chess::Kind::Queen);
    CHECK(decoded.color == Chess::Color::Black);
    CHECK(decoded.cell == Position{4, 3});
}

// Animation-event fields (see CaptureEvent's own comment) - these are what
// the client's capture-sound timing actually keys off, so a round-trip
// mismatch here would silently break when the sound plays.
TEST_CASE("CaptureEventCodec - encode/decode משמרים את שדות ה-animation-event") {
    CaptureEvent original{ Chess::Kind::Pawn, Chess::Color::White, Position{2, 2}, 12, 7, Position{5, 5}, 0.63 };

    CaptureEvent decoded = CaptureEventCodec::decode(CaptureEventCodec::encode(original));

    CHECK(decoded.capturedPieceId == 12);
    CHECK(decoded.capturingPieceId == 7);
    CHECK(decoded.collisionCell == Position{5, 5});
    CHECK(decoded.impactProgress == doctest::Approx(0.63));
}

TEST_CASE("CaptureEventCodec - encode מייצר JSON שטוח עם kind/color/cell") {
    CaptureEvent event{ Chess::Kind::Knight, Chess::Color::White, Position{0, 1}, 1, 2, Position{0, 1}, 1.0 };
    nlohmann::json j = CaptureEventCodec::encode(event);

    CHECK(j.at("kind") == "Knight");
    CHECK(j.at("color") == "White");
    CHECK(j.at("cell").at("row") == 0);
    CHECK(j.at("cell").at("col") == 1);
    CHECK(j.at("capturedPieceId") == 1);
    CHECK(j.at("capturingPieceId") == 2);
    CHECK(j.at("impactProgress") == doctest::Approx(1.0));
}

TEST_CASE("CaptureEventCodec - כל סוגי הכלים עוברים round-trip") {
    for (Chess::Kind kind : { Chess::Kind::King, Chess::Kind::Queen, Chess::Kind::Rook,
                               Chess::Kind::Bishop, Chess::Kind::Knight, Chess::Kind::Pawn }) {
        CaptureEvent original{ kind, Chess::Color::White, Position{1, 1}, 1, 2, Position{1, 1}, 1.0 };
        CaptureEvent decoded = CaptureEventCodec::decode(CaptureEventCodec::encode(original));
        CHECK(decoded.kind == kind);
    }
}

TEST_CASE("CaptureEventCodec - decode עם צבע לא מוכר זורק חריגה") {
    nlohmann::json j = { {"kind", "Pawn"}, {"color", "Purple"}, {"cell", {{"row", 0}, {"col", 0}}} };
    CHECK_THROWS(CaptureEventCodec::decode(j));
}

TEST_CASE("CaptureEventCodec - decode עם סוג כלי לא מוכר זורק חריגה") {
    nlohmann::json j = { {"kind", "Dragon"}, {"color", "White"}, {"cell", {{"row", 0}, {"col", 0}}} };
    CHECK_THROWS(CaptureEventCodec::decode(j));
}
