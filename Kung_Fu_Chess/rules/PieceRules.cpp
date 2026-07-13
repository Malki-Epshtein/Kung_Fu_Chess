#include "PieceRules.h"
#include "RookRules.h"
#include "BishopRules.h"
#include "QueenRules.h"
#include "KnightRules.h"
#include "KingRules.h"
#include "PawnRules.h"

std::vector<Position> PieceRules::legalDestinations(const Board& board, const Piece& piece, bool ignoreBlockers) {
    switch (piece.getKind()) {
        case Chess::Kind::Rook:   return RookRules::moves(board, piece, ignoreBlockers);
        case Chess::Kind::Bishop: return BishopRules::moves(board, piece, ignoreBlockers);
        case Chess::Kind::Queen:  return QueenRules::moves(board, piece, ignoreBlockers);
        case Chess::Kind::Knight: return KnightRules::moves(board, piece);
        case Chess::Kind::King:   return KingRules::moves(board, piece);
        case Chess::Kind::Pawn:   return PawnRules::moves(board, piece);
        default:                  return {};
    }
}
