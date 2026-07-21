#pragma once
#include <string>

// One completed move, ready to log or display. MoveLogObserver (server-side
// only) produces these; MoveLogCodec turns them into wire JSON in both
// directions. Lives in shared/, not server/engine/, because it's the wire
// contract between server and client - not an internal engine detail.
struct MoveEntry {
    std::string timestamp; // elapsed simulation time (game_clock_ms) since the room's game started, "MM:SS.mmm"
    std::string notation;  // simplified algebraic notation, e.g. "Nc6", "exd5"
};
