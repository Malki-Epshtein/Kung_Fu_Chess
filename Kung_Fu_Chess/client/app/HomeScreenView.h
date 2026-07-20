#pragma once
#include "../net/WsClient.h"
#include <string>

struct HomeScreenResult {
    // false only if the user closed the window/pressed ESC without ever
    // successfully joining a room - GraphicalApplication treats that as
    // "quit the app", same as closing the game window today.
    bool        joinedRoom = false;
    std::string roomName;
};

// Opens the Home Screen window (Play/Room buttons) and blocks until the
// user either successfully creates/joins a room, or closes the window
// (ESC or the window's close button). Play doesn't lead anywhere yet
// (Stage H). Needs OpenCV, so - like GraphicalApplication/ImageView -
// this stays out of the Win32 test build.
HomeScreenResult runHomeScreen(WsClient& client);
