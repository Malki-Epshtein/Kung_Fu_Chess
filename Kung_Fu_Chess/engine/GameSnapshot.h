#pragma once
#include <vector>
#include "../model/Position.h"
#include "../model/Piece.h"

struct SnapshotPiece {
    Chess::Kind  kind;
    Chess::Color color;
    Position     cell;
    Chess::State state;
    int          elapsed_in_state_ms;
    Position     targetCell;     // destination cell if moving; equals cell otherwise
    double       travelProgress; // 0.0-1.0 fraction of the move completed; 0 if not moving
};

struct GameSnapshot {
    int                        board_width;
    int                        board_height;
    std::vector<SnapshotPiece> pieces;
    bool                       game_over;
    bool                       has_selection = false;
    Position                   selected_cell{};
};
