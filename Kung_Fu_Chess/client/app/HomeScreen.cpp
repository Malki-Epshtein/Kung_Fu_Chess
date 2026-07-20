#include "HomeScreen.h"

namespace {
    bool contains(const ButtonBounds& b, int x, int y) {
        return x >= b.x && x < b.x + b.width && y >= b.y && y < b.y + b.height;
    }
}

ButtonBounds HomeScreen::playButtonBounds() {
    return { 300, 150, 200, 60 };
}

ButtonBounds HomeScreen::roomButtonBounds() {
    return { 300, 260, 200, 60 };
}

HomeScreenChoice HomeScreen::hitTest(int x, int y) {
    if (contains(playButtonBounds(), x, y))
        return HomeScreenChoice::Play;
    if (contains(roomButtonBounds(), x, y))
        return HomeScreenChoice::Room;
    return HomeScreenChoice::None;
}
