#pragma once
#include "../model/Piece.h"
#include "../model/Position.h"

// One focused interface (Interface Segregation) - anyone who only cares
// about completed moves implements just this, nothing else.
class MoveObserver {
public:
    virtual void onMoveCompleted(const Piece& mover, Position from, Position to) = 0;
    virtual ~MoveObserver() = default;
};
