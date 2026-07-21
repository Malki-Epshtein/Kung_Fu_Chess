#pragma once
#include "../net/WsClient.h"
#include "json.hpp"
#include <string>

struct HomeScreenResult {
    // false only if the user closed the window/pressed ESC without ever
    // successfully joining a room - GraphicalApplication treats that as
    // "quit the app", same as closing the game window today.
    bool        joinedRoom = false;
    std::string roomName;

    // The room's move history so far (MoveLogCodec::encodeAll shape) - empty
    // for a brand new room, populated when joining/matching into a game
    // already in progress. GraphicalApplication seeds its local move log
    // from this once, before opening the game window.
    nlohmann::json moveLog = { {"white", nlohmann::json::array()}, {"black", nlohmann::json::array()} };
};

// Opens the Home Screen window (Play/Room buttons) and blocks until the
// user either successfully creates/joins a room, or closes the window
// (ESC or the window's close button). Play doesn't lead anywhere yet
// (Stage H). Needs OpenCV, so - like GraphicalApplication/ImageView -
// this stays out of the Win32 test build.
HomeScreenResult runHomeScreen(WsClient& client);
