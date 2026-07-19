#pragma once
#include <vector>
#include <utility>
#include "Motion.h"

// Pure geometry helpers for straight-line motion, independent of Board or
// any RealTimeArbiter state - one cell per 1000ms, Chebyshev distance.
int calcTravelTime(Position from, Position to);

// Enumerates the (cell, simulated-ms) checkpoints a straight-line motion
// passes through, one cell per 1000ms starting from its own start_time_ms.
// Empty for a jump (from == to, no path to walk).
std::vector<std::pair<Position, int>> pathCheckpoints(const Motion& m);
