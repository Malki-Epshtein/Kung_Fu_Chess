#pragma once
#include <vector>
#include "../model/Position.h"
#include "../model/Piece.h"

struct SnapshotPiece {
    Chess::Kind  kind;
    Chess::Color color;
    Position     cell;
    Chess::State state;
};

struct GameSnapshot {
    int                        board_width;
    int                        board_height;
    std::vector<SnapshotPiece> pieces;
    bool                       game_over;
};
