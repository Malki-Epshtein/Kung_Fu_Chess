#pragma once
#include "../realtime/CaptureObserver.h"
#include "../../shared/protocol/CaptureEvent.h"
#include <functional>

// Implements only CaptureObserver (Interface Segregation) - same principle
// as ScoreObserver, but this one's only job is to notify a listener that a
// capture happened (for client-side sound/animation triggers), not to
// tally anything. Kept separate from ScoreObserver so score-keeping never
// has to know the network layer exists, matching how MoveLogObserver's
// onNewEntry hook keeps move-logging separate from network delivery.
class CaptureEventObserver : public CaptureObserver {
public:
    // Fired right after a capture is observed. Empty by default (no-op).
    std::function<void(const CaptureEvent&)> onNewCapture;

    void onPieceCaptured(const Piece& captured, const CaptureImpact& impact) override;
};
