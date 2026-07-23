#pragma once
#include <vector>
#include <memory>
#include "Piece.h"

// Kept concrete on purpose - not behind an IBoard interface. There's
// exactly one real implementation (constructed in server/io/BoardParser.cpp)
// and no substitution actually happens anywhere: tests already build real
// Board instances directly (cheap, deterministic, no I/O - nothing to fake),
// and a future Replay feature should replay recorded moves/snapshots
// through a real GameEngine+Board rather than need a different Board type.
// GameEngine already receives its board via constructor injection
// (GameEngine(std::shared_ptr<Board>) - see GameEngine.h) - only the TYPE
// is concrete, not the wiring.
// Revisit this if a second real implementation actually needs to exist
// (e.g. a bitboard for AI move generation) - and prefer a template
// parameter over a virtual interface even then: Board's methods
// (getPiece/isWithinBounds/movePiece) are called on every move validation,
// every tick, so virtual dispatch there costs real inlining opportunities
// in the engine's hottest path for a substitution that's decided once, at
// compile time, by which code path constructs the board - not something
// that needs to vary at runtime.
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
    std::shared_ptr<Piece> getPieceById(int id) const;

    bool isCellEmpty(Position pos) const;
    bool isWithinBounds(Position pos) const;
    int getWidth()  const { return width;  }
    int getHeight() const { return height; }
    std::shared_ptr<Piece> getPieceAt(int row, int col) const {
        return isWithinBounds({ row, col }) ? grid[index(row, col)] : nullptr;
    }


    void movePiece(Position from, Position to);
};