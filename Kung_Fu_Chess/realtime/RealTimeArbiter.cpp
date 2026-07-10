#include "RealTimeArbiter.h"

int RealTimeArbiter::calcTravelTime(Position from, Position to) {
    int dr = std::abs(to.row - from.row);
    int dc = std::abs(to.col  - from.col);
    return std::max(dr, dc) * 1000;
}

void RealTimeArbiter::addMotion(Position from, Position to, int piece_id) {
    int arrival = game_clock_ms + calcTravelTime(from, to);
    active_motions.push_back({ from, to, arrival, piece_id });
}

bool RealTimeArbiter::tick(int ms) {
    game_clock_ms += ms;
    bool king_captured = false;

    for (auto it = active_motions.begin(); it != active_motions.end();) {
        if (it->arrival_time_ms > game_clock_ms) { ++it; continue; }

        auto piece = board.getPiece(it->from);
        if (piece && piece->getId() == it->piece_id) {
            auto target = board.getPiece(it->to);
            if (target && target->getKind() == Chess::Kind::King)
                king_captured = true;
            board.movePiece(it->from, it->to);
        }

        it = active_motions.erase(it);
    }

    return king_captured;
}
