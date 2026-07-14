#include "MoveLogObserver.h"
#include <sstream>

namespace {
    std::string kindName(Chess::Kind kind) {
        switch (kind) {
            case Chess::Kind::King:   return "King";
            case Chess::Kind::Queen:  return "Queen";
            case Chess::Kind::Rook:   return "Rook";
            case Chess::Kind::Bishop: return "Bishop";
            case Chess::Kind::Knight: return "Knight";
            case Chess::Kind::Pawn:   return "Pawn";
            default:                  return "";
        }
    }

    std::string colorName(Chess::Color color) {
        return color == Chess::Color::White ? "White" : "Black";
    }
}

std::string MoveLogObserver::describe(const Piece& mover, Position from, Position to) {
    std::ostringstream out;
    out << colorName(mover.getColor()) << " " << kindName(mover.getKind())
        << ": " << from << " -> " << to;
    return out.str();
}

void MoveLogObserver::onMoveCompleted(const Piece& mover, Position from, Position to) {
    std::string entry = describe(mover, from, to);

    if (mover.getColor() == Chess::Color::White)
        whiteMoves.push_back(entry);
    else if (mover.getColor() == Chess::Color::Black)
        blackMoves.push_back(entry);
}

const std::vector<std::string>& MoveLogObserver::getMoves(Chess::Color color) const {
    static const std::vector<std::string> empty;
    if (color == Chess::Color::White) return whiteMoves;
    if (color == Chess::Color::Black) return blackMoves;
    return empty;
}
