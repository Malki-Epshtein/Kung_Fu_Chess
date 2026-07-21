#pragma once
#include <string>

// The real-world identity behind a room's three roles - White's and
// Black's username/elo, plus how many spectators are currently watching
// (a count, not a name list - see RoomIdentityResolver). An unfilled seat
// (room not yet joined by a second player) leaves that side's fields at
// their defaults (empty username, elo 0).
struct RoomIdentity {
    std::string whiteUsername;
    int         whiteElo = 0;
    std::string blackUsername;
    int         blackElo = 0;
    int         spectatorCount = 0;
};
