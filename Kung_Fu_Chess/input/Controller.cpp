#include "Controller.h"

static const SnapshotPiece* findPieceAt(const GameSnapshot& snap, Position pos) {
    for (const auto& p : snap.pieces)
        if (p.cell == pos)
            return &p;
    return nullptr;
}

void Controller::handleMouseClick(int x, int y) {
    GameSnapshot snap = engine.snapshot();

    int boardWidthPx  = snap.board_width  * BoardMapper::CELL_SIZE;
    int boardHeightPx = snap.board_height * BoardMapper::CELL_SIZE;

    if (x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx) {
        selectedPos = nullptr;
        return;
    }

    Position clickedPos = BoardMapper::mapToPosition(x, y);

    if (selectedPos == nullptr) {
        if (findPieceAt(snap, clickedPos))
            selectedPos = std::make_shared<Position>(clickedPos);
    }
    else {
        const SnapshotPiece* clickedPiece  = findPieceAt(snap, clickedPos);
        const SnapshotPiece* selectedPiece = findPieceAt(snap, *selectedPos);

        if (clickedPiece && selectedPiece &&
            clickedPiece->color == selectedPiece->color &&
            clickedPiece->color != Chess::Color::None) {
            selectedPos = std::make_shared<Position>(clickedPos);
        }
        else {
            engine.requestMove(*selectedPos, clickedPos);
            selectedPos = nullptr;
        }
    }
}

void Controller::handleJump(int x, int y) {
    GameSnapshot snap = engine.snapshot();

    int boardWidthPx  = snap.board_width  * BoardMapper::CELL_SIZE;
    int boardHeightPx = snap.board_height * BoardMapper::CELL_SIZE;

    if (x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx)
        return;

    Position pos = BoardMapper::mapToPosition(x, y);
    engine.requestJump(pos);
}

void Controller::handleWait(int ms) {
    engine.wait(ms);
}

GameSnapshot Controller::getSnapshot() const {
    GameSnapshot snap = engine.snapshot();
    if (selectedPos != nullptr) {
        snap.has_selection = true;
        snap.selected_cell = *selectedPos;
    }
    return snap;
}
