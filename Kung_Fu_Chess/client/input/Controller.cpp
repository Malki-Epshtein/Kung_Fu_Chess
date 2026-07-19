#include "Controller.h"
#include "../view/ViewConfig.h"
#include "../../server/input/MoveCommand.h"
#include "../../server/input/JumpCommand.h"

static const SnapshotPiece* findPieceAt(const GameSnapshot& snap, Position pos) {
    for (const auto& p : snap.pieces)
        if (p.cell == pos)
            return &p;
    return nullptr;
}

void Controller::handleMouseClick(int x, int y) {
    GameSnapshot snap = engine.snapshot();

    int boardWidthPx  = snap.board_width  * ViewConfig::CELL_SIZE;
    int boardHeightPx = snap.board_height * ViewConfig::CELL_SIZE;

    if (x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx) {
        selectedPos = std::nullopt;
        return;
    }

    Position clickedPos = BoardMapper::mapToPosition(x, y);

    if (!selectedPos.has_value()) {
        if (findPieceAt(snap, clickedPos))
            selectedPos = clickedPos;
    }
    else if (clickedPos == selectedPos.value()) {
        // Second click on the already-selected piece itself - jump in place.
        JumpCommand(clickedPos).execute(engine);
        selectedPos = std::nullopt;
    }
    else {
        const SnapshotPiece* clickedPiece  = findPieceAt(snap, clickedPos);
        const SnapshotPiece* selectedPiece = findPieceAt(snap, selectedPos.value());

        if (clickedPiece && selectedPiece &&
            clickedPiece->color == selectedPiece->color &&
            clickedPiece->color != Chess::Color::None &&
            selectedPiece->kind != Chess::Kind::Knight) {
            selectedPos = clickedPos;
        }
        else {
            MoveCommand(selectedPos.value(), clickedPos).execute(engine);
            selectedPos = std::nullopt;
        }
    }
}

void Controller::handleJump(int x, int y) {
    GameSnapshot snap = engine.snapshot();

    int boardWidthPx  = snap.board_width  * ViewConfig::CELL_SIZE;
    int boardHeightPx = snap.board_height * ViewConfig::CELL_SIZE;

    if (x < 0 || y < 0 || x >= boardWidthPx || y >= boardHeightPx)
        return;

    Position pos = BoardMapper::mapToPosition(x, y);
    JumpCommand(pos).execute(engine);
}

void Controller::handleWait(int ms) {
    engine.wait(ms);
}

GameSnapshot Controller::getSnapshot() const {
    GameSnapshot snap = engine.snapshot();//יהי בקונטרולר snapshot של המשחק
    if (selectedPos.has_value()) {
        snap.has_selection = true;
        snap.selected_cell = selectedPos.value();
        snap.legalMoves = engine.legalDestinationsFrom(selectedPos.value());//בשביל החייל שנבחר, אני שולף את כל התאים החוקיים שהוא יכול להגיע אליהם
    }
    return snap;
}
