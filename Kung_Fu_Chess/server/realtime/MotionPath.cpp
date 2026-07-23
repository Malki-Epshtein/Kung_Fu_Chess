#include "MotionPath.h"
#include <algorithm>
#include <cmath>

namespace {
    // How long a piece takes to cross one board square - the game's own
    // "movement speed" tuning knob, shared by both functions below so they
    // can never drift out of sync with each other.
    constexpr int MS_PER_SQUARE_TRAVELED = 1000;
}

int calcTravelTime(Position from, Position to) {
    int dr = std::abs(to.row - from.row);
    int dc = std::abs(to.col - from.col);
    return std::max(dr, dc) * MS_PER_SQUARE_TRAVELED;
}

std::vector<std::pair<Position, int>> pathCheckpoints(const Motion& m) {
    std::vector<std::pair<Position, int>> result;
    int totalDr = m.to.row - m.from.row;
    int totalDc = m.to.col - m.from.col;
    int steps = std::max(std::abs(totalDr), std::abs(totalDc));
    if (steps == 0) return result;
    int dr = totalDr / steps;
    int dc = totalDc / steps;
    for (int i = 1; i <= steps; ++i) {
        Position cell{ m.from.row + dr * i, m.from.col + dc * i };
        result.push_back({ cell, m.start_time_ms + i * MS_PER_SQUARE_TRAVELED });
    }
    return result;
}
