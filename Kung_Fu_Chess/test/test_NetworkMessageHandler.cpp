#include "../doctest.h"
#include "../client/net/NetworkMessageHandler.h"

// isCaptureSoundReady is the pure predicate behind
// NetworkMessageHandler::checkPendingCaptureSounds() - pulled out precisely
// so this bug (reported as "black eating white plays no sound") is locked
// under a real test instead of only fixed by inspection. Root cause: a
// motion is erased from RealTimeArbiter's active_motions the same tick it
// arrives, so travelProgress resets to 0.0 the very first snapshot that
// could ever show the watched piece "finished" - a normal arrival capture's
// impactProgress==1.0 was therefore unreachable via travelProgress alone.

TEST_CASE("isCaptureSoundReady - כלי שסיים לזוז (לא עוד Moving) מוכן גם אם travelProgress אפס") {
    SnapshotPiece watched;
    watched.state = Chess::State::Idle;
    watched.travelProgress = 0.0;

    // This is exactly the bug scenario: an arrival capture recorded
    // impactProgress==1.0, but the watched piece's motion already finished
    // and travelProgress reset to 0.0 before this check ever ran.
    CHECK(NetworkMessageHandler::isCaptureSoundReady(&watched, 1.0));
}

TEST_CASE("isCaptureSoundReady - כלי שעדיין Moving וטרם הגיע להתקדמות הפגיעה - לא מוכן") {
    SnapshotPiece watched;
    watched.state = Chess::State::Moving;
    watched.travelProgress = 0.3;

    CHECK_FALSE(NetworkMessageHandler::isCaptureSoundReady(&watched, 0.8));
}

TEST_CASE("isCaptureSoundReady - כלי שעדיין Moving והגיע/עבר את התקדמות הפגיעה - מוכן") {
    SnapshotPiece watched;
    watched.state = Chess::State::Moving;
    watched.travelProgress = 0.8;

    CHECK(NetworkMessageHandler::isCaptureSoundReady(&watched, 0.8));
}

TEST_CASE("isCaptureSoundReady - כלי שנעלם לגמרי מה-snapshot מוכן מיד") {
    CHECK(NetworkMessageHandler::isCaptureSoundReady(nullptr, 1.0));
}
