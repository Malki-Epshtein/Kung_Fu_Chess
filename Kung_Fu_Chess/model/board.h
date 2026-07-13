#pragma once
#include <vector>
#include <memory> 
#include "Piece.h"

class Board {
private:
    int width;
    int height;

    std::vector<std::shared_ptr<Piece>> grid; // מערך שטוח: תא (row,col) יושב באינדקס row*width+col
    int index(int row, int col) const { return row * width + col; }

public:
    Board(int width, int height);


    void addPiece(std::shared_ptr<Piece> piece, Position pos);
    void removePiece(Position pos);
    std::shared_ptr<Piece> getPiece(Position pos) const;

    bool isCellEmpty(Position pos) const;
    bool isWithinBounds(Position pos) const;
    int getWidth()  const { return width;  }
    int getHeight() const { return height; }
    std::shared_ptr<Piece> getPieceAt(int row, int col) const { return grid[index(row, col)]; }


    void movePiece(Position from, Position to);
};