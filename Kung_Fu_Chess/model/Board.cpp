#include "Board.h"

static std::shared_ptr<Piece> makeEmpty(Position pos) {
    return std::make_shared<EmptyPiece>(pos);
}

Board::Board(int width, int height)
    : width(width), height(height) {
    grid.resize(height);
    for (int r = 0; r < height; r++) {
        grid[r].resize(width);
        for (int c = 0; c < width; c++)
            grid[r][c] = makeEmpty({ r, c });
    }
}

void Board::addPiece(std::shared_ptr<Piece> piece, Position pos) {
    if (isWithinBounds(pos) && isCellEmpty(pos))
        grid[pos.row][pos.col] = piece;
}

void Board::removePiece(Position pos) {
    if (isWithinBounds(pos))
        grid[pos.row][pos.col] = makeEmpty(pos);
}

std::shared_ptr<Piece> Board::getPiece(Position pos) const {
    if (!isWithinBounds(pos)) return nullptr;
    return grid[pos.row][pos.col];
}

bool Board::isCellEmpty(Position pos) const {
    if (!isWithinBounds(pos)) return false;
    return grid[pos.row][pos.col]->getKind() == Chess::Kind::None;
}

bool Board::isWithinBounds(Position pos) const {
    return pos.row >= 0 && pos.row < height &&
           pos.col >= 0 && pos.col < width;
}

void Board::movePiece(Position from, Position to) {
    if (!isWithinBounds(from) || !isWithinBounds(to)) return;
    grid[to.row][to.col] = grid[from.row][from.col];
    grid[to.row][to.col]->setCell(to);
    grid[from.row][from.col] = makeEmpty(from);
}
