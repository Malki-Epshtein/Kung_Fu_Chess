#pragma once
#include "../net/WsClient.h"
#include "../../shared/model/Piece.h"
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

    // White/Black's real identity, resolved server-side (RoomIdentityResolver)
    // at the moment of this reply - empty name/elo 0 if that seat isn't
    // filled yet. A one-time snapshot, not kept live here (Stage I's own
    // "score/disconnect ride the periodic snapshot" pattern is what keeps
    // this current once the game window is open).
    std::string whiteName;
    int         whiteElo = 0;
    std::string blackName;
    int         blackElo = 0;

    // This connection's own seated color in the room just joined - drives
    // Controller::setMyColor (see its comment) so a player can never select
    // the opponent's pieces. None both when the server said "Spectator" and
    // as this struct's own default (a spectator should never be able to
    // select anything either).
    Chess::Color role = Chess::Color::None;
};

// Opens the Home Screen window (Play/Room buttons) and blocks until the
// user either successfully creates/joins a room, or closes the window
// (ESC or the window's close button). Play doesn't lead anywhere yet
// (Stage H). Needs OpenCV, so - like GraphicalApplication/ImageView -
// this stays out of the Win32 test build.
HomeScreenResult runHomeScreen(WsClient& client);
