#pragma once
#include "MoveEntry.h"
#include "../model/Piece.h"
#include "json.hpp"
#include <vector>

// One decoded MOVE_LOGGED push: which side moved, plus the move itself.
struct MoveLogEvent {
    Chess::Color color;
    MoveEntry    entry;
};

// A room's full move history so far, split by color - what a connection
// joining a room already in progress needs to backfill its local move log
// before it starts receiving live MOVE_LOGGED pushes.
struct MoveLogBundle {
    std::vector<MoveEntry> white;
    std::vector<MoveEntry> black;
};

// Wire format for move-log data. encode/decode is one move, sent as the
// payload of a single MOVE_LOGGED push each time a move completes.
// encodeAll/decodeAll is the full history, embedded as one field on a
// room-join reply (JoinRoom/FindGame/EnterRoom) - not its own message type,
// since it only ever rides along on an existing reply.
class MoveLogCodec {
public:
    static nlohmann::json encode(Chess::Color color, const MoveEntry& entry);
    static MoveLogEvent   decode(const nlohmann::json& j);

    static nlohmann::json encodeAll(const std::vector<MoveEntry>& whiteMoves, const std::vector<MoveEntry>& blackMoves);
    static MoveLogBundle  decodeAll(const nlohmann::json& j);
};
