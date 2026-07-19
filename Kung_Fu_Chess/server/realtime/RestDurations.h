#pragma once

// Shared real-time timing constants - previously private to RealTimeArbiter,
// now factored out so CollisionResolver and ArrivalResolver (both split out
// of RealTimeArbiter) can use the same values without RealTimeArbiter having
// to hand them out as constructor/method parameters everywhere.
namespace RestDurations {
    constexpr int SHORT_REST_MS    = 500;  // rest after a jump
    constexpr int LONG_REST_MS     = 1000; // rest after any other action
    constexpr int JUMP_DURATION_MS = 1000; // time a jump spends airborne
}
