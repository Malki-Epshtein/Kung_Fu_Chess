#include "../doctest.h"
#include "../view/assets/AssetPathBuilder.h"

TEST_CASE("AssetPathBuilder - configPath בונה נתיב לפי תיקיית כלי/צבע/מצב") {
    AssetPathBuilder builder("assets");

    CHECK(builder.configPath(Chess::Kind::Pawn, Chess::Color::White, Chess::State::Idle)
          == "assets/PW/states/idle/config.json");
    CHECK(builder.configPath(Chess::Kind::Knight, Chess::Color::Black, Chess::State::LongRest)
          == "assets/NB/states/long_rest/config.json");
}

TEST_CASE("AssetPathBuilder - spritePath ממספר את הפריימים החל מ-1, לא מ-0") {
    AssetPathBuilder builder("assets");

    CHECK(builder.spritePath(Chess::Kind::King, Chess::Color::White, Chess::State::Moving, 0)
          == "assets/KW/states/move/sprites/1.png");
    CHECK(builder.spritePath(Chess::Kind::King, Chess::Color::White, Chess::State::Moving, 4)
          == "assets/KW/states/move/sprites/5.png");
}

TEST_CASE("AssetPathBuilder - כל סוגי הכלים ומצבי המשחק ממופים לתיקיה תקינה") {
    AssetPathBuilder builder("assets");

    CHECK(builder.spritePath(Chess::Kind::Queen,  Chess::Color::Black, Chess::State::Jump,      0) == "assets/QB/states/jump/sprites/1.png");
    CHECK(builder.spritePath(Chess::Kind::Rook,   Chess::Color::White, Chess::State::ShortRest, 0) == "assets/RW/states/short_rest/sprites/1.png");
    CHECK(builder.spritePath(Chess::Kind::Bishop, Chess::Color::Black, Chess::State::Idle,       0) == "assets/BB/states/idle/sprites/1.png");
}

TEST_CASE("AssetPathBuilder - סוג כלי ללא ייצוג גרפי (None) זורק חריגה במקום להחזיר נתיב שגוי") {
    AssetPathBuilder builder("assets");
    CHECK_THROWS_AS(builder.configPath(Chess::Kind::None, Chess::Color::White, Chess::State::Idle),
                     std::invalid_argument);
}
