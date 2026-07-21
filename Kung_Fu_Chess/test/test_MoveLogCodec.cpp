#include "../doctest.h"
#include "../shared/protocol/MoveLogCodec.h"

TEST_CASE("MoveLogCodec - encode/decode של מהלך בודד משמרים צבע וזמן ונוטציה") {
    MoveEntry entry{ "01:12.345", "e4" };

    nlohmann::json encoded = MoveLogCodec::encode(Chess::Color::White, entry);
    MoveLogEvent decoded = MoveLogCodec::decode(encoded);

    CHECK(decoded.color == Chess::Color::White);
    CHECK(decoded.entry.timestamp == "01:12.345");
    CHECK(decoded.entry.notation == "e4");
}

TEST_CASE("MoveLogCodec - encode מייצר JSON שטוח עם color/timestamp/notation") {
    MoveEntry entry{ "00:00.500", "Nc6" };
    nlohmann::json j = MoveLogCodec::encode(Chess::Color::Black, entry);

    CHECK(j.at("color") == "Black");
    CHECK(j.at("timestamp") == "00:00.500");
    CHECK(j.at("notation") == "Nc6");
}

TEST_CASE("MoveLogCodec - decode עם צבע לא מוכר זורק חריגה") {
    nlohmann::json j = { {"color", "Purple"}, {"timestamp", "00:00.000"}, {"notation", "e4"} };
    CHECK_THROWS(MoveLogCodec::decode(j));
}

TEST_CASE("MoveLogCodec - encodeAll/decodeAll משמרים את שני הצדדים בנפרד") {
    std::vector<MoveEntry> white{ {"00:00.100", "e4"}, {"00:05.200", "Nf3"} };
    std::vector<MoveEntry> black{ {"00:02.300", "e5"} };

    nlohmann::json encoded = MoveLogCodec::encodeAll(white, black);
    MoveLogBundle decoded = MoveLogCodec::decodeAll(encoded);

    REQUIRE(decoded.white.size() == 2);
    CHECK(decoded.white[0].notation == "e4");
    CHECK(decoded.white[1].notation == "Nf3");
    REQUIRE(decoded.black.size() == 1);
    CHECK(decoded.black[0].notation == "e5");
}

TEST_CASE("MoveLogCodec - encodeAll עם רשימות ריקות עובר round-trip לרשימות ריקות") {
    MoveLogBundle decoded = MoveLogCodec::decodeAll(MoveLogCodec::encodeAll({}, {}));

    CHECK(decoded.white.empty());
    CHECK(decoded.black.empty());
}

TEST_CASE("MoveLogCodec - סדר המהלכים נשמר (לא ממוין מחדש)") {
    std::vector<MoveEntry> white{ {"00:00.100", "a3"}, {"00:01.100", "a4"}, {"00:02.100", "a5"} };

    MoveLogBundle decoded = MoveLogCodec::decodeAll(MoveLogCodec::encodeAll(white, {}));

    REQUIRE(decoded.white.size() == 3);
    CHECK(decoded.white[0].notation == "a3");
    CHECK(decoded.white[1].notation == "a4");
    CHECK(decoded.white[2].notation == "a5");
}
