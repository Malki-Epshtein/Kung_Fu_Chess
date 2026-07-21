#pragma once
#include "../../shared/model/Piece.h"
#include "../../shared/model/Position.h"

// One focused interface (Interface Segregation) - anyone who only cares
// about completed moves implements just this, nothing else.
//
// gameClockMs is the room's own RealTimeArbiter simulation clock at the
// moment this move resolved - not wall-clock time. Real-time multiplayer
// games log event timestamps against the deterministic simulation clock,
// not the OS clock, precisely so logged times stay consistent with
// everything else already measured in game_clock_ms (motion arrival,
// cooldown expiry, animation progress).
class MoveObserver {
public://כדי שלכולם יהי את השעון שלי
    virtual void onMoveCompleted(const Piece& mover, Position from, Position to, bool wasCapture, int gameClockMs) = 0;
    virtual ~MoveObserver() = default;
};
