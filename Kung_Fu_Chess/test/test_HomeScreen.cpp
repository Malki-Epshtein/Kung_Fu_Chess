#include "../doctest.h"
#include "../client/app/HomeScreen.h"

TEST_CASE("HomeScreen - קליק בתוך כפתור Play מזוהה כ-Play") {
    ButtonBounds b = HomeScreen::playButtonBounds();
    CHECK(HomeScreen::hitTest(b.x + 1, b.y + 1) == HomeScreenChoice::Play);
    CHECK(HomeScreen::hitTest(b.x + b.width - 1, b.y + b.height - 1) == HomeScreenChoice::Play);
}

TEST_CASE("HomeScreen - קליק בתוך כפתור Room מזוהה כ-Room") {
    ButtonBounds b = HomeScreen::roomButtonBounds();
    CHECK(HomeScreen::hitTest(b.x + 1, b.y + 1) == HomeScreenChoice::Room);
    CHECK(HomeScreen::hitTest(b.x + b.width - 1, b.y + b.height - 1) == HomeScreenChoice::Room);
}

TEST_CASE("HomeScreen - קליק מחוץ לשני הכפתורים מזוהה כ-None") {
    CHECK(HomeScreen::hitTest(0, 0) == HomeScreenChoice::None);
    CHECK(HomeScreen::hitTest(700, 400) == HomeScreenChoice::None);
}

TEST_CASE("HomeScreen - הגבול הימני/תחתון של כפתור (exclusive) כבר לא בתוכו") {
    ButtonBounds b = HomeScreen::playButtonBounds();
    CHECK(HomeScreen::hitTest(b.x + b.width, b.y) == HomeScreenChoice::None);
    CHECK(HomeScreen::hitTest(b.x, b.y + b.height) == HomeScreenChoice::None);
}

TEST_CASE("HomeScreen - כפתורי Play ו-Room לא חופפים") {
    ButtonBounds play = HomeScreen::playButtonBounds();
    ButtonBounds room = HomeScreen::roomButtonBounds();
    bool overlapsVertically = play.y < room.y + room.height && room.y < play.y + play.height;
    CHECK_FALSE(overlapsVertically);
}
