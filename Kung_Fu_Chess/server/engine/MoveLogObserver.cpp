#include "MoveLogObserver.h"
#include <sstream>
#include <iomanip>

namespace {
    // Simplified algebraic notation: no check ("+"), no disambiguation
    // (e.g. "Nbd7"), no castling/promotion suffix - the project doesn't
    // implement check detection or castling at all, and disambiguation
    // would require MoveLogObserver to depend on a live Board, which it's
    // deliberately free of (Interface Segregation).
    std::string pieceLetter(Chess::Kind kind) {
        switch (kind) {
            case Chess::Kind::King:   return "K";
            case Chess::Kind::Queen:  return "Q";
            case Chess::Kind::Rook:   return "R";
            case Chess::Kind::Bishop: return "B";
            case Chess::Kind::Knight: return "N";
            default:                  return ""; // Pawn (and None)
        }
    }

    std::string fileLetter(int col) {
        return std::string(1, static_cast<char>('a' + col));
    }
}

std::string MoveLogObserver::describe(const Piece& mover, Position from, Position to, bool wasCapture) const {
    std::ostringstream out;
    out << pieceLetter(mover.getKind());

    // A pawn capture shows its origin file even without disambiguation
    // ("exd5"), a rule of real algebraic notation, not a simplification.
    if (mover.getKind() == Chess::Kind::Pawn && wasCapture)
        out << fileLetter(from.col);

    if (wasCapture)
        out << "x";

    out << fileLetter(to.col) << (boardHeight - to.row);
    return out.str();
}

std::string MoveLogObserver::formatElapsed(int gameClockMs) {
    long long totalMs = gameClockMs;
    long long minutes = totalMs / 60000;
    long long seconds = (totalMs / 1000) % 60;
    long long millis  = totalMs % 1000;

    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds << "."
        << std::setfill('0') << std::setw(3) << millis;
    return out.str();
}

void MoveLogObserver::onMoveCompleted(const Piece& mover, Position from, Position to, bool wasCapture, int gameClockMs) {
    MoveEntry entry{ formatElapsed(gameClockMs), describe(mover, from, to, wasCapture) };

    Chess::Color color = mover.getColor();
    if (color == Chess::Color::White)
        whiteMoves.push_back(entry);
    else if (color == Chess::Color::Black)
        blackMoves.push_back(entry);
    else
        return; // shouldn't happen in practice - a piece always has a real color

    if (onNewEntry)
        onNewEntry(color, entry);
}

const std::vector<MoveEntry>& MoveLogObserver::getMoves(Chess::Color color) const {
    static const std::vector<MoveEntry> empty;
    if (color == Chess::Color::White) return whiteMoves;
    if (color == Chess::Color::Black) return blackMoves;
    return empty;
}
