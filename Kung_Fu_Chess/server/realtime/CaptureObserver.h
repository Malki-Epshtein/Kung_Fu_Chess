#pragma once
#include "../../shared/model/Piece.h"
#include "../../shared/model/Position.h"

// Animation-event-style capture-sound timing (see CaptureEvent's own
// comment for the full client-side rationale): the server stays
// authoritative for WHETHER a capture happened, but also hands over enough
// data for the client to decide WHEN it becomes visible, instead of the
// client just reacting to "a message arrived."
//
// capturingPieceId - the piece whose live travelProgress the client should
// watch (usually the survivor of a collision, or the piece that just
// arrived in a normal capture-on-arrival). May equal the captured piece's
// own id in the one case where the captured piece is itself the one that
// was mid-motion and gets removed the same tick (see ArrivalResolver) -
// harmless, since that piece will simply be absent from the very next
// snapshot, which the client already treats as "fire immediately."
// collisionCell - where the capture visually happened.
// impactProgress - capturingPieceId's own travelProgress (0.0-1.0) at the
// exact moment of impact; 1.0 for a normal arrival capture (the piece is
// captured the instant it lands, i.e. at the end of its own motion).
struct CaptureImpact {
    int      capturingPieceId;
    Position collisionCell;
    double   impactProgress;
};

// One focused interface (Interface Segregation) - anyone who only cares
// about captures implements just this, nothing else. Observers that don't
// care about sound/animation timing (e.g. ScoreObserver) simply ignore
// `impact`.
class CaptureObserver {
public:
    virtual void onPieceCaptured(const Piece& captured, const CaptureImpact& impact) = 0;
    virtual ~CaptureObserver() = default;
};
