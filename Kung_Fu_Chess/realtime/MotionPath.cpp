#include "MotionPath.h"
#include <algorithm>
#include <cmath>

int calcTravelTime(Position from, Position to) {
    int dr = std::abs(to.row - from.row);
    int dc = std::abs(to.col - from.col);
    return std::max(dr, dc) * 1000;
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
        result.push_back({ cell, m.start_time_ms + i * 1000 });
    }
    return result;
}
