#pragma once
#include "../model/Piece.h"
#include "../model/Position.h"

// One piece capture, as reported to clients - not part of GameSnapshot
// (a captured piece is simply absent from it) and not carried inside
// MoveEntry's notation string either (that's display text, not a
// structured signal). This is the dedicated wire event a client-side
// sound/animation trigger can react to.
struct CaptureEvent {
    Chess::Kind  kind;
    Chess::Color color;
    Position     cell;
};
