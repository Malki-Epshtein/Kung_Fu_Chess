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

void RealTimeArbiter::addJump(Position pos, int piece_id) {
    active_motions.push_back({ pos, pos, game_clock_ms + 1000, piece_id });
}

bool RealTimeArbiter::tick(int ms) {
    game_clock_ms += ms;
    bool king_captured = false;

    // Snapshot of everything arriving this tick, taken before any mutation,
    // so a jump landing this same tick is still visible to the capture check below.
    std::vector<Motion> arriving;
    for (const auto& m : active_motions)
        if (m.arrival_time_ms <= game_clock_ms)
            arriving.push_back(m);

    for (const auto& m : arriving) {
        if (m.from == m.to)
            continue; // jump landing: the piece never left its cell, nothing to resolve here

        auto piece = board.getPiece(m.from);
        if (!piece || piece->getId() != m.piece_id)
            continue;

        bool capturedByJump = false;
        for (const auto& other : active_motions) {
            if (other.from == other.to && other.from == m.to &&
                other.arrival_time_ms >= game_clock_ms) {
                auto jumper = board.getPiece(other.from);
                if (jumper && jumper->getColor() != piece->getColor()) {
                    capturedByJump = true;
                    break;
                }
            }
        }

        if (capturedByJump) {
            board.removePiece(m.from);
        }
        else {
            auto target = board.getPiece(m.to);
            if (target && target->getKind() == Chess::Kind::King)
                king_captured = true;
            board.movePiece(m.from, m.to);

            if (piece->getKind() == Chess::Kind::Pawn) {
                int promotionRow = (piece->getColor() == Chess::Color::White) ? 0 : board.getHeight() - 1;
                if (m.to.row == promotionRow)
                    piece->promoteTo(Chess::Kind::Queen);
            }
        }
    }

    for (auto it = active_motions.begin(); it != active_motions.end();) {
        if (it->arrival_time_ms <= game_clock_ms)
            it = active_motions.erase(it);
        else
            ++it;
    }

    return king_captured;
}
