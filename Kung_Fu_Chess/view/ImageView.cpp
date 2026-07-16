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

    const SnapshotPiece* pieceAt(const GameSnapshot& snapshot, Position pos) {
        for (const auto& piece : snapshot.pieces)
            if (piece.cell == pos)
                return &piece;
        return nullptr;
    }

    // BGR - orange for the long rest after a normal move, light green for
    // the short rest after a jump, so the two cooldown kinds read as
    // visually distinct at a glance.
    const cv::Scalar LONG_REST_HIGHLIGHT(0, 165, 255);
    const cv::Scalar SHORT_REST_HIGHLIGHT(144, 238, 144);

    // BGR - dark, semi-transparent dot marking a legal destination for the
    // currently selected piece, in the style of common chess UIs.
    const cv::Scalar LEGAL_MOVE_MARKER(60, 60, 60);

    // A bordered panel with a title header, name/score lines, and a
    // "Time | Move" table beneath - one drawn per side (Black/White).
    void drawPlayerPanel(Img& frame, int panelX, const std::string& title,
        const std::string& playerName, int score, const std::vector<MoveEntry>& moves)
    {
        const cv::Scalar borderColor(0, 0, 0);
        const cv::Scalar headerBg(70, 70, 70);
        const cv::Scalar bodyBg(50, 50, 50);
        const cv::Scalar textColor(255, 255, 255);
        const int panelWidth = 220; // must fit both panels within canvas.png's 1360px width
        const int panelY     = 10;
        const int headerH    = 28;
        const int infoH      = 40;
        const int lineHeight = 18;
        const int maxLines   = 20;
        int totalHeight = headerH + infoH + maxLines * lineHeight;

        // Panel background, then a thin border on all four sides.
        frame.fill_rect(panelX, panelY, panelWidth, totalHeight, bodyBg, 1.0);
        frame.fill_rect(panelX, panelY, panelWidth, 2, borderColor, 1.0);
        frame.fill_rect(panelX, panelY + totalHeight - 2, panelWidth, 2, borderColor, 1.0);
        frame.fill_rect(panelX, panelY, 2, totalHeight, borderColor, 1.0);
        frame.fill_rect(panelX + panelWidth - 2, panelY, 2, totalHeight, borderColor, 1.0);

        // Header bar with the side's title.
        frame.fill_rect(panelX, panelY, panelWidth, headerH, headerBg, 1.0);
        frame.put_text(title, panelX + panelWidth / 2 - 20, panelY + headerH - 9, 0.6, textColor);

        int y = panelY + headerH + 18;
        frame.put_text("Name: " + playerName, panelX + 8, y, 0.45, textColor);
        y += 20;
        frame.put_text("Score: " + std::to_string(score), panelX + 8, y, 0.45, textColor);
        y += 20;

        // "Time | Move" column headers, then a rule line under them.
        frame.put_text("Time", panelX + 8, y, 0.4, textColor);
        frame.put_text("Move", panelX + 90, y, 0.4, textColor);
        y += lineHeight;
        frame.fill_rect(panelX + 4, y - 12, panelWidth - 8, 1, borderColor, 1.0);
        y += 4;

        int start = std::max(0, static_cast<int>(moves.size()) - maxLines);
        for (int i = start; i < static_cast<int>(moves.size()); ++i) {
            frame.put_text(moves[i].timestamp, panelX + 8, y, 0.35, textColor);
            frame.put_text(moves[i].notation, panelX + 90, y, 0.35, textColor);
            y += lineHeight;
        }
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

    // Board sits centered, offset right by the left panel's width and down
    // by BOARD_MARGIN - canvas.png has that much real, empty space reserved
    // above and below the board specifically so the frame/labels below have
    // somewhere to go that isn't on top of the board itself.
    int boardX = ViewConfig::PANEL_WIDTH;
    int boardY = ViewConfig::BOARD_MARGIN;
    background.draw_on(frame, boardX, boardY);

    // Black frame drawn entirely in the reserved margin, around the board -
    // never overlapping it - with coordinates printed inside the frame.
    const cv::Scalar FRAME_COLOR(0, 0, 0);
    const cv::Scalar LABEL_COLOR(255, 255, 255);
    int boardW = snapshot.board_width  * ViewConfig::CELL_SIZE;
    int boardH = snapshot.board_height * ViewConfig::CELL_SIZE;
    int margin = ViewConfig::BOARD_MARGIN;

    frame.fill_rect(boardX, boardY - margin, boardW, margin, FRAME_COLOR, 1.0);          // top band
    frame.fill_rect(boardX, boardY + boardH, boardW, margin, FRAME_COLOR, 1.0);          // bottom band
    frame.fill_rect(boardX - margin, boardY - margin, margin, boardH + 2 * margin, FRAME_COLOR, 1.0); // left band (incl. corners)
    frame.fill_rect(boardX + boardW, boardY - margin, margin, boardH + 2 * margin, FRAME_COLOR, 1.0); // right band (incl. corners)

    // Files (A-H) in the top and bottom bands; ranks (1-8) in the left and
    // right bands. Rank 1 is the bottom row (row = board_height-1), matching
    // how White's back rank is set up in the starting position.
    for (int c = 0; c < snapshot.board_width; ++c) {
        std::string file(1, static_cast<char>('A' + c));
        int labelX = boardX + c * ViewConfig::CELL_SIZE + ViewConfig::CELL_SIZE / 2 - 6;
        frame.put_text(file, labelX, boardY - margin / 2 + 5, 0.5, LABEL_COLOR);
        frame.put_text(file, labelX, boardY + boardH + margin / 2 + 5, 0.5, LABEL_COLOR);
    }
    for (int r = 0; r < snapshot.board_height; ++r) {
        std::string rank = std::to_string(snapshot.board_height - r);
        int labelY = boardY + r * ViewConfig::CELL_SIZE + ViewConfig::CELL_SIZE / 2 + 5;
        frame.put_text(rank, boardX - margin + 8, labelY, 0.5, LABEL_COLOR);
        frame.put_text(rank, boardX + boardW + margin / 2 - 4, labelY, 0.5, LABEL_COLOR);
    }

    // Legal-move markers for the currently selected piece - computed by
    // GameEngine::legalDestinationsFrom (which delegates straight to
    // PieceRules::legalDestinations), never recomputed here. That set can
    // still include friendly-occupied cells (Knight rule 8, or section-7
    // "maybe it moves away in time" for King/sliding pieces) - the rule
    // itself is unchanged, but a dot suggesting "free to go here" would be
    // misleading on a cell that's only speculatively reachable, so those are
    // filtered out here, in the view only. Drawn before the pieces so a
    // marker on an enemy-occupied cell sits under that piece's sprite rather
    // than covering it.
    if (snapshot.has_selection) {
        const SnapshotPiece* selectedPiece = pieceAt(snapshot, snapshot.selected_cell);
        for (const auto& dest : snapshot.legalMoves) {
            const SnapshotPiece* occupant = pieceAt(snapshot, dest);
            if (occupant && selectedPiece && occupant->color == selectedPiece->color)
                continue;

            int centerX = boardX + dest.col * ViewConfig::CELL_SIZE + ViewConfig::CELL_SIZE / 2;
            int centerY = boardY + dest.row * ViewConfig::CELL_SIZE + ViewConfig::CELL_SIZE / 2;
            frame.fill_circle(centerX, centerY, ViewConfig::CELL_SIZE / 6, LEGAL_MOVE_MARKER, 0.5);
        }
    }

    for (const auto& piece : snapshot.pieces) {
        if (piece.kind == Chess::Kind::None)
            continue;

        // Render interpolation (Game Loop pattern): the piece's logical cell
        // only updates on arrival, but travelProgress (0-1, from GameEngine)
        // lets the draw position slide smoothly between origin and target
        // instead of jumping there when the motion resolves. boardX/boardY
        // shift everything to match the now-framed, centered board.
        int fromX = boardX + piece.cell.col * ViewConfig::CELL_SIZE;
        int fromY = boardY + piece.cell.row * ViewConfig::CELL_SIZE;
        int toX   = boardX + piece.targetCell.col * ViewConfig::CELL_SIZE;
        int toY   = boardY + piece.targetCell.row * ViewConfig::CELL_SIZE;
        int drawX = fromX + static_cast<int>((toX - fromX) * piece.travelProgress);
        int drawY = fromY + static_cast<int>((toY - fromY) * piece.travelProgress);

        // Resting pieces highlight their own cell - orange for the long rest
        // after a normal move, light green for the short rest after a jump -
        // drawn under the sprite so the piece itself stays fully visible.
        // The fill starts full and drains downward as restProgress (0-1,
        // from GameEngine) advances, so the remaining cooldown reads at a
        // glance instead of just a flat on/off color block.
        if (piece.state == Chess::State::LongRest || piece.state == Chess::State::ShortRest) {
            const cv::Scalar& color = (piece.state == Chess::State::LongRest)
                ? LONG_REST_HIGHLIGHT : SHORT_REST_HIGHLIGHT;
            int fillHeight = static_cast<int>(ViewConfig::CELL_SIZE * (1.0 - piece.restProgress));
            frame.fill_rect(fromX, fromY + (ViewConfig::CELL_SIZE - fillHeight), ViewConfig::CELL_SIZE, fillHeight, color);
        }

        Img sprite = spriteSource.getSprite(piece.kind, piece.color, piece.state, piece.elapsed_in_state_ms);
        sprite.draw_on(frame, drawX, drawY);
    }

    // Side panels: Black on the left, White on the right - each shows its
    // own name, score, and move history as a "Time | Move" table. Comes
    // from Observer objects (RealTimeArbiter -> observers -> here), never
    // through GameSnapshot - see the Step 9 plan.
    const int leftPanelX  = 10;
    const int rightPanelX = ViewConfig::PANEL_WIDTH + ViewConfig::BOARD_WIDTH_PX + ViewConfig::BOARD_MARGIN + 15;

    if (scoreObserver && moveLogObserver) {
        drawPlayerPanel(frame, leftPanelX, "Black", blackPlayerName,
            scoreObserver->getScore(Chess::Color::Black), moveLogObserver->getMoves(Chess::Color::Black));
        drawPlayerPanel(frame, rightPanelX, "White", whitePlayerName,
            scoreObserver->getScore(Chess::Color::White), moveLogObserver->getMoves(Chess::Color::White));
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
        int textY = ViewConfig::BOARD_MARGIN + boardPixelHeight / 2;
        frame.put_text(message, textX, textY, 1.0);
    }

    // Not using Img::show() here - it blocks on waitKey(0) every call, which
    // would freeze a live render loop. GraphicalApplication owns the event
    // pump (waitKey) and calls render() once per frame instead.
    cv::imshow(ViewConfig::WINDOW_NAME, frame.get_mat());
}
