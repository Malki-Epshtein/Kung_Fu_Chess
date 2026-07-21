#include "../doctest.h"
#include "../server/app/session/GameResultCodec.h"

TEST_CASE("GameResultCodec - encode/decode משמרים את כל השדות (KingCapture)") {
    GameResult original{ Chess::Color::White, GameEndReason::KingCapture, "alice", 1250, "bob", 1400 };

    GameResult decoded = GameResultCodec::decode(GameResultCodec::encode(original));

    CHECK(decoded.winner == Chess::Color::White);
    CHECK(decoded.reason == GameEndReason::KingCapture);
    CHECK(decoded.winnerUsername == "alice");
    CHECK(decoded.winnerElo == 1250);
    CHECK(decoded.loserUsername == "bob");
    CHECK(decoded.loserElo == 1400);
}

TEST_CASE("GameResultCodec - encode/decode משמרים Disconnect כסיבה") {
    GameResult original{ Chess::Color::Black, GameEndReason::Disconnect, "bob", 1400, "alice", 1250 };

    GameResult decoded = GameResultCodec::decode(GameResultCodec::encode(original));

    CHECK(decoded.winner == Chess::Color::Black);
    CHECK(decoded.reason == GameEndReason::Disconnect);
}

TEST_CASE("GameResultCodec - decode עם צבע לא מוכר זורק חריגה") {
    nlohmann::json j = { {"winner", "Purple"}, {"reason", "KingCapture"},
                          {"winnerUsername", "a"}, {"winnerElo", 1200}, {"loserUsername", "b"}, {"loserElo", 1200} };
    CHECK_THROWS(GameResultCodec::decode(j));
}

TEST_CASE("GameResultCodec - decode עם סיבת סיום לא מוכרת זורק חריגה") {
    nlohmann::json j = { {"winner", "White"}, {"reason", "Timeout"},
                          {"winnerUsername", "a"}, {"winnerElo", 1200}, {"loserUsername", "b"}, {"loserElo", 1200} };
    CHECK_THROWS(GameResultCodec::decode(j));
}
