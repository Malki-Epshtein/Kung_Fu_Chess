#pragma once
#include "../realtime/CaptureObserver.h"

// Implements only CaptureObserver (Interface Segregation) - score only
// cares about captures, never about moves.
class ScoreObserver : public CaptureObserver {
private:
    int whiteScore = 0;
    int blackScore = 0;

    static int pieceValue(Chess::Kind kind);

public:
    void onPieceCaptured(const Piece& captured, const CaptureImpact& impact) override;
    int  getScore(Chess::Color color) const;
};
