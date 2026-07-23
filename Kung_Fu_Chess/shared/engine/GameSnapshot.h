#pragma once
#include <vector>
#include "../model/Position.h"
#include "../model/Piece.h"

struct SnapshotPiece {
    int          id; // stable server-side piece id (Piece::getId()) - lets a
                      // client correlate the same piece across snapshots,
                      // e.g. to watch a specific piece's live travelProgress
                      // (see NetworkMessageHandler's capture-sound timing)
    Chess::Kind  kind;
    Chess::Color color;
    Position     cell;
    Chess::State state;
    int          elapsed_in_state_ms;
    Position     targetCell;     // destination cell if moving; equals cell otherwise
    double       travelProgress; // 0.0-1.0 fraction of the move completed; 0 if not moving
    double       restProgress;   // 0.0-1.0 fraction of ShortRest/LongRest elapsed; 0 otherwise
};

struct GameSnapshot {
    int                        board_width;
    int                        board_height;
    std::vector<SnapshotPiece> pieces;
    bool                       game_over;
    bool                       has_selection = false;
    Position                   selected_cell{};
    std::vector<Position>      legalMoves; // legal destinations for the selected piece; empty if no selection
};
