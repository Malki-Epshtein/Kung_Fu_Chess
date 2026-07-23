#pragma once
#include "../model/Piece.h"
#include "../model/Position.h"

// One piece capture, as reported to clients - not part of GameSnapshot
// (a captured piece is simply absent from it) and not carried inside
// MoveEntry's notation string either (that's display text, not a
// structured signal). This is the dedicated wire event a client-side
// sound/animation trigger reacts to.
//
// The server stays authoritative for WHETHER a capture happened (this
// event only ever gets sent once that's already decided); everything below
// kind/color/cell exists purely so the CLIENT can decide WHEN to actually
// trigger the sound - the real Animation-Event/Notify technique (Unity/
// Unreal): a trigger tied to a specific point along an animation's own
// playback, evaluated every frame, rather than fired the instant the
// underlying game-logic event occurred. Without this, a capture from two
// pieces crossing paths mid-flight would sound the instant their paths
// cross server-side, which can be well before anything visibly collides -
// the surviving piece keeps sliding toward its own destination for a while
// after the capture is already final.
//
//   capturedPieceId - the id of the piece that was captured (redundant
//     with kind/color/cell for display purposes, but explicit and robust
//     for a client that wants to key off id instead).
//   capturingPieceId - the id of the piece whose live travelProgress the
//     client should watch each frame (see NetworkMessageHandler).
//   collisionCell - where the capture visually happened.
//   impactProgress - capturingPieceId's own travelProgress (0.0-1.0) at
//     the exact moment of impact. The client fires the sound once that
//     piece's live travelProgress (already computed every tick for
//     rendering - see GameSnapshot) reaches this value. 1.0 for a normal
//     arrival capture, since the piece is captured the instant it lands -
//     the end of its own motion.
struct CaptureEvent {
    Chess::Kind  kind;
    Chess::Color color;
    Position     cell;
    int          capturedPieceId;
    int          capturingPieceId;
    Position     collisionCell;
    double       impactProgress;
};
