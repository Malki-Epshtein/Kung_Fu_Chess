#include "BoardPrinter.h"

void BoardPrinter::print(const Board& board, std::ostream& out) {
    for (int r = 0; r < board.getHeight(); ++r) {
        for (int c = 0; c < board.getWidth(); ++c) {
            auto piece = board.getPieceAt(r, c);
            if (!piece || piece->getKind() == Chess::Kind::None) {
                out << ".";
            } else {
                out << (piece->getColor() == Chess::Color::White ? 'w' : 'b');
                switch (piece->getKind()) {
                    case Chess::Kind::Pawn:   out << 'P'; break;
                    case Chess::Kind::Rook:   out << 'R'; break;
                    case Chess::Kind::Knight: out << 'N'; break;
                    case Chess::Kind::Bishop: out << 'B'; break;
                    case Chess::Kind::Queen:  out << 'Q'; break;
                    case Chess::Kind::King:   out << 'K'; break;
                    default: break;
                }
            }
            if (c < board.getWidth() - 1) out << " ";
        }
        out << "\n";
    }
}
