#include "ImageView.h"
#include <algorithm>

namespace {
    // A king is only ever missing from the snapshot once it has been
    // captured, so whichever color's king is still present is the winner.
    Chess::Color survivingKingColor(const GameSnapshot& snapshot) {
        bool whiteKing = false, blackKing = false;
        for (const auto& piece : snapshot.pieces) {
            if (piece.kind != Chess::Kind::King) continue;
            if (piece.color == Chess::Color::White) whiteKing = true;
            else if (piece.color == Chess::Color::Black) blackKing = true;
        }
        if (whiteKing && !blackKing) return Chess::Color::White;
        if (blackKing && !whiteKing) return Chess::Color::Black;
        return Chess::Color::None;
    }
}

void ImageView::render(const GameSnapshot& snapshot) {
    if (!backgroundLoaded) {
        background.read(assetsRoot + "/board.png");
        backgroundLoaded = true;
    }
    if (!canvasLoaded) {
        canvas.read(assetsRoot + "/canvas.png");
        canvasLoaded = true;
    }

    // Fresh deep copy every frame - canvas is the cached, reusable "clean"
    // background (side panels + empty board area); draw_on mutates its
    // target in place, so drawing directly onto the cached Img would
    // permanently bake pieces/text into it.
    Img frame = canvas.clone();

    // Board sits centered, offset right by the left panel's width.
    background.draw_on(frame, ViewConfig::PANEL_WIDTH, 0);

    for (const auto& piece : snapshot.pieces) {
        if (piece.kind == Chess::Kind::None)
            continue;

        Img sprite = spriteSource.getSprite(piece.kind, piece.color, piece.state, piece.elapsed_in_state_ms);

        // Render interpolation (Game Loop pattern): the piece's logical cell
        // only updates on arrival, but travelProgress (0-1, from GameEngine)
        // lets the draw position slide smoothly between origin and target
        // instead of jumping there when the motion resolves. PANEL_WIDTH
        // shifts everything right to match the now-centered board.
        int fromX = ViewConfig::PANEL_WIDTH + piece.cell.col * ViewConfig::CELL_SIZE;
        int fromY = piece.cell.row * ViewConfig::CELL_SIZE;
        int toX   = ViewConfig::PANEL_WIDTH + piece.targetCell.col * ViewConfig::CELL_SIZE;
        int toY   = piece.targetCell.row * ViewConfig::CELL_SIZE;
        int drawX = fromX + static_cast<int>((toX - fromX) * piece.travelProgress);
        int drawY = fromY + static_cast<int>((toY - fromY) * piece.travelProgress);

        sprite.draw_on(frame, drawX, drawY);
    }

    // Side panels: Black on the left, White on the right - each shows its
    // own score and recent move history. Comes from Observer objects
    // (RealTimeArbiter -> observers -> here), never through GameSnapshot -
    // see the Step 9 plan. Uses put_text's default color so this file stays
    // free of cv:: types.
    const int leftPanelX  = 10;
    const int rightPanelX = ViewConfig::PANEL_WIDTH + ViewConfig::BOARD_WIDTH_PX + 15;
    const int lineHeight  = 20;
    const int maxLines    = 15;

    if (scoreObserver) {
        frame.put_text("Black", leftPanelX, 25, 0.7);
        frame.put_text("Score: " + std::to_string(scoreObserver->getScore(Chess::Color::Black)), leftPanelX, 55, 0.5);

        frame.put_text("White", rightPanelX, 25, 0.7);
        frame.put_text("Score: " + std::to_string(scoreObserver->getScore(Chess::Color::White)), rightPanelX, 55, 0.5);
    }

    if (moveLogObserver) {
        const auto& blackMoves = moveLogObserver->getMoves(Chess::Color::Black);
        int startBlack = std::max(0, static_cast<int>(blackMoves.size()) - maxLines);
        int y = 90;
        for (int i = startBlack; i < static_cast<int>(blackMoves.size()); ++i) {
            frame.put_text(blackMoves[i], leftPanelX, y, 0.35);
            y += lineHeight;
        }

        const auto& whiteMoves = moveLogObserver->getMoves(Chess::Color::White);
        int startWhite = std::max(0, static_cast<int>(whiteMoves.size()) - maxLines);
        y = 90;
        for (int i = startWhite; i < static_cast<int>(whiteMoves.size()); ++i) {
            frame.put_text(whiteMoves[i], rightPanelX, y, 0.35);
            y += lineHeight;
        }
    }

    // Spec (Iteration 9): display an end-of-game message when game_over is true.
    if (snapshot.game_over) {
        std::string message = "Game Over";
        Chess::Color winner = survivingKingColor(snapshot);
        if (winner == Chess::Color::White) message += " - White Wins!";
        else if (winner == Chess::Color::Black) message += " - Black Wins!";

        int boardPixelWidth  = snapshot.board_width  * ViewConfig::CELL_SIZE;
        int boardPixelHeight = snapshot.board_height * ViewConfig::CELL_SIZE;
        int textX = ViewConfig::PANEL_WIDTH + boardPixelWidth / 2 - 180;
        int textY = boardPixelHeight / 2;
        frame.put_text(message, textX, textY, 1.0);
    }

    // Not using Img::show() here - it blocks on waitKey(0) every call, which
    // would freeze a live render loop. GraphicalApplication owns the event
    // pump (waitKey) and calls render() once per frame instead.
    cv::imshow(ViewConfig::WINDOW_NAME, frame.get_mat());
}
