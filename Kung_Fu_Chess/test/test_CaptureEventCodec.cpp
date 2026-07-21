#include "../doctest.h"
#include "../shared/protocol/CaptureEventCodec.h"

TEST_CASE("CaptureEventCodec - encode/decode משמרים סוג, צבע ומשבצת") {
    CaptureEvent original{ Chess::Kind::Queen, Chess::Color::Black, Position{4, 3} };

    CaptureEvent decoded = CaptureEventCodec::decode(CaptureEventCodec::encode(original));

    CHECK(decoded.kind == Chess::Kind::Queen);
    CHECK(decoded.color == Chess::Color::Black);
    CHECK(decoded.cell == Position{4, 3});
}

TEST_CASE("CaptureEventCodec - encode מייצר JSON שטוח עם kind/color/cell") {
    CaptureEvent event{ Chess::Kind::Knight, Chess::Color::White, Position{0, 1} };
    nlohmann::json j = CaptureEventCodec::encode(event);

    CHECK(j.at("kind") == "Knight");
    CHECK(j.at("color") == "White");
    CHECK(j.at("cell").at("row") == 0);
    CHECK(j.at("cell").at("col") == 1);
}

TEST_CASE("CaptureEventCodec - כל סוגי הכלים עוברים round-trip") {
    for (Chess::Kind kind : { Chess::Kind::King, Chess::Kind::Queen, Chess::Kind::Rook,
                               Chess::Kind::Bishop, Chess::Kind::Knight, Chess::Kind::Pawn }) {
        CaptureEvent original{ kind, Chess::Color::White, Position{1, 1} };
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
