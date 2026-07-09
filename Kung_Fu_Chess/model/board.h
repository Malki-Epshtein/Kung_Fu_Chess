#pragma once
#include <vector>
#include <memory> // ������ �-std::shared_ptr ������ ������ �� ����
#include "Piece.h"

class Board {
private:
    int width;
    int height;
    // ������ �� ������� �����. nullptr ����� �� ��� (�� ����� ������ �������� Piece �� Kind::None)
    std::vector<std::vector<std::shared_ptr<Piece>>> grid;

public:
    Board(int width, int height);

    // ������ ������� ������ ����
    void addPiece(std::shared_ptr<Piece> piece, Position pos);
    void removePiece(Position pos);
    std::shared_ptr<Piece> getPiece(Position pos) const;

    bool isCellEmpty(Position pos) const;
    bool isWithinBounds(Position pos) const;
    int getWidth()  const { return width;  }
    int getHeight() const { return height; }
    std::shared_ptr<Piece> getPieceAt(int row, int col) const { return grid[row][col]; }

    // ���� ���
    void movePiece(Position from, Position to);
};