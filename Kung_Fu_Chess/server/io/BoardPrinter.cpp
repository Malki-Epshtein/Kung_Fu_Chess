#include "BoardPrinter.h"
#include <vector>

void BoardPrinter::print(const GameState& state, std::ostream& out) {
    const Board& board = *state.board;
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

void BoardPrinter::print(const GameSnapshot& snapshot, std::ostream& out) {
    std::vector<std::vector<const SnapshotPiece*>> grid(
        snapshot.board_height, std::vector<const SnapshotPiece*>(snapshot.board_width, nullptr));

    for (const auto& piece : snapshot.pieces)
        grid[piece.cell.row][piece.cell.col] = &piece;

    for (int r = 0; r < snapshot.board_height; ++r) {
        for (int c = 0; c < snapshot.board_width; ++c) {
            const SnapshotPiece* piece = grid[r][c];
            if (!piece) {
                out << ".";
            } else {
                out << (piece->color == Chess::Color::White ? 'w' : 'b');
                switch (piece->kind) {
                    case Chess::Kind::Pawn:   out << 'P'; break;
                    case Chess::Kind::Rook:   out << 'R'; break;
                    case Chess::Kind::Knight: out << 'N'; break;
                    case Chess::Kind::Bishop: out << 'B'; break;
                    case Chess::Kind::Queen:  out << 'Q'; break;
                    case Chess::Kind::King:   out << 'K'; break;
                    default: break;
                }
            }
            if (c < snapshot.board_width - 1) out << " ";
        }
        out << "\n";
    }
}
