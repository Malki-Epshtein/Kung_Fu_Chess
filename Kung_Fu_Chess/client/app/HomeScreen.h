#pragma once

// Pure click-region logic for the Home Screen's two buttons (Play, Room) -
// zero OpenCV dependency, so this stays unit-testable exactly like
// BoardMapper. The actual window/rendering loop lives in HomeScreenView
// (needs OpenCV). Stage G2a: Play/Room don't lead anywhere yet - this only
// identifies which button, if any, a click landed on.
enum class HomeScreenChoice { None, Play, Room };

struct ButtonBounds {
    int x;
    int y;
    int width;
    int height;
};

class HomeScreen {
public:
    static ButtonBounds playButtonBounds();
    static ButtonBounds roomButtonBounds();
    static HomeScreenChoice hitTest(int x, int y);
};
