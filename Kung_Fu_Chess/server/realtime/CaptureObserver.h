#pragma once
#include "../../shared/model/Piece.h"

// One focused interface (Interface Segregation) - anyone who only cares
// about captures implements just this, nothing else.
class CaptureObserver {
public:
    virtual void onPieceCaptured(const Piece& captured) = 0;
    virtual ~CaptureObserver() = default;
};
