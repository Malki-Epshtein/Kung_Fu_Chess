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

    // drawPlayerPanel's own palette - named here alongside the rest of this
    // file's colors instead of as local variables inside the function.
    const cv::Scalar PANEL_BORDER_COLOR(0, 0, 0);
    const cv::Scalar PANEL_HEADER_BG(70, 70, 70);
    const cv::Scalar PANEL_BODY_BG(50, 50, 50);
    const cv::Scalar PANEL_TEXT_COLOR(255, 255, 255);

    // Shared with the Spectators count below (see render()) so it can be
    // placed under the Black panel instead of guessing at its height.
    constexpr int PANEL_Y      = 10;
    constexpr int PANEL_HEADER_H = 28;
    constexpr int PANEL_INFO_H   = 40;
    constexpr int PANEL_LINE_H   = 18;
    constexpr int PANEL_MAX_LINES = 20;
    constexpr int PANEL_TOTAL_H  = PANEL_HEADER_H + PANEL_INFO_H + PANEL_MAX_LINES * PANEL_LINE_H;

    // Shared with render()'s canvas-size computation below, same reason as
    // the PANEL_* constants above.
    constexpr int PLAYER_PANEL_WIDTH = 220;

    // The gap between the board's edge and a player panel - used on both
    // sides of the right panel (once in the canvas-size computation, once
    // in the panel's own x-position), so it's named once here instead of
    // risking the two literals drifting apart.
    constexpr int PLAYER_PANEL_GAP_PX = 15;

    // A bordered panel with a title header, name/score lines, and a
    // "Time | Move" table beneath - one drawn per side (Black/White).
    void drawPlayerPanel(Img& frame, int panelX, const std::string& title,
        const std::string& playerName, int score, const std::vector<MoveEntry>& moves)
    {
        const cv::Scalar& borderColor = PANEL_BORDER_COLOR;
        const cv::Scalar& headerBg    = PANEL_HEADER_BG;
        const cv::Scalar& bodyBg      = PANEL_BODY_BG;
        const cv::Scalar& textColor   = PANEL_TEXT_COLOR;
        const int panelWidth = PLAYER_PANEL_WIDTH;
        const int panelY     = PANEL_Y;
        const int headerH    = PANEL_HEADER_H;
        const int infoH      = PANEL_INFO_H;
        const int lineHeight = PANEL_LINE_H;
        const int maxLines   = PANEL_MAX_LINES;
        int totalHeight = PANEL_TOTAL_H;

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
    int cell = boardScale.cellSize();
    int boardW = snapshot.board_width  * cell;
    int boardH = snapshot.board_height * cell;

    // Reload board.png and canvas.png only when the cell size actually
    // changed (not every frame). board.png is sized to exactly fill the
    // current grid - same technique SpriteRepository already uses to size
    // piece sprites to one cell. canvas.png is a plain backdrop (every
    // panel/label/piece is painted on top of it fresh each frame anyway),
    // so stretching it to a newly-computed total window size is cosmetically
    // fine - it's just resized to contain the board at its new size plus
    // both side panels, which keep their own fixed pixel width.
    //
    // The very first frame(s) after the window opens can still have a
    // default-constructed, all-zero snapshot (no real one has arrived from
    // the server yet) - board_width/height would be 0 then, and Img::read
    // treats a {0,0} target size as "don't resize" rather than "resize to
    // zero", so board.png would stay at its native size while canvas.png
    // (never zero, since it also adds the fixed panel/margin widths) still
    // shrank to fit a 0-wide board - the two would then disagree and
    // draw_on below would throw. Falling back to 8x8 here for sizing
    // purposes only keeps both reloads consistent with each other even
    // before the real board size is known.
    int reloadCols = snapshot.board_width  > 0 ? snapshot.board_width  : 8;
    int reloadRows = snapshot.board_height > 0 ? snapshot.board_height : 8;
    int reloadBoardW = reloadCols * cell;
    int reloadBoardH = reloadRows * cell;
    if (cell != lastRenderedCellSize) {
        background.read(assetsRoot + "/board.png", { reloadBoardW, reloadBoardH });

        int totalW = ViewConfig::PANEL_WIDTH + reloadBoardW + ViewConfig::BOARD_MARGIN + PLAYER_PANEL_GAP_PX + PLAYER_PANEL_WIDTH + PLAYER_PANEL_GAP_PX;
        int totalH = ViewConfig::BOARD_MARGIN + reloadBoardH + ViewConfig::BOARD_MARGIN;
        canvas.read(assetsRoot + "/canvas.png", { totalW, totalH });

        lastRenderedCellSize = cell;
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
        int labelX = boardX + c * cell + cell / 2 - 6;
        frame.put_text(file, labelX, boardY - margin / 2 + 5, 0.5, LABEL_COLOR);
        frame.put_text(file, labelX, boardY + boardH + margin / 2 + 5, 0.5, LABEL_COLOR);
    }
    for (int r = 0; r < snapshot.board_height; ++r) {
        std::string rank = std::to_string(snapshot.board_height - r);
        int labelY = boardY + r * cell + cell / 2 + 5;
        frame.put_text(rank, boardX - margin + 8, labelY, 0.5, LABEL_COLOR);
        frame.put_text(rank, boardX + boardW + margin / 2 - 4, labelY, 0.5, LABEL_COLOR);
    }

    // Legal-move markers for the currently selected piece - computed by
    // GameEngine::legalDestinationsFrom (which delegates straight to
    // PieceRules::legalDestinations), never recomputed here. That set can
    // include friendly-occupied cells for two different reasons: a Knight
    // (rule 8) can ALWAYS eat a friendly piece there - a real, unconditional
    // destination, so its dot stays. Section 7 ("maybe it moves away in
    // time") lets King/sliding pieces merely request a currently-friendly
    // cell speculatively - not guaranteed, so a dot there would be
    // misleading and is filtered out, view-only, for every kind but Knight.
    if (snapshot.has_selection) {
        const SnapshotPiece* selectedPiece = pieceAt(snapshot, snapshot.selected_cell);
        bool selectedIsKnight = selectedPiece && selectedPiece->kind == Chess::Kind::Knight;
        for (const auto& dest : snapshot.legalMoves) {
            const SnapshotPiece* occupant = pieceAt(snapshot, dest);
            bool friendlyOccupied = occupant && selectedPiece && occupant->color == selectedPiece->color;
            if (friendlyOccupied && !selectedIsKnight)
                continue;

            int centerX = boardX + dest.col * cell + cell / 2;
            int centerY = boardY + dest.row * cell + cell / 2;
            frame.fill_circle(centerX, centerY, cell / 6, LEGAL_MOVE_MARKER, 0.5);
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
        int fromX = boardX + piece.cell.col * cell;
        int fromY = boardY + piece.cell.row * cell;
        int toX   = boardX + piece.targetCell.col * cell;
        int toY   = boardY + piece.targetCell.row * cell;
        int drawX = fromX + static_cast<int>((toX - fromX) * piece.travelProgress);
        int drawY = fromY + static_cast<int>((toY - fromY) * piece.travelProgress);

        // Resting pieces highlight their own cell - orange for the long rest
        // after a normal move, light green for the short rest after a jump -
        // drawn under the sprite so the piece itself stays fully visible.
        // The fill starts full and drains downward as restProgress (0-1,
        // from GameEngine) advances, so the remaining cooldown reads at a
        // glance instead of just a flat on/off color block.
        if (piece.state == Chess::State::LongRest || piece.state == Chess::State::ShortRest) {
            const cv::Scalar& color = (piece.state == Chess::State::LongRest)//תצבע לפי התא
                ? LONG_REST_HIGHLIGHT : SHORT_REST_HIGHLIGHT;
            int fillHeight = static_cast<int>(cell * (1.0 - piece.restProgress));
            frame.fill_rect(fromX, fromY + (cell - fillHeight), cell, fillHeight, color);
        }

        Img sprite = spriteSource.getSprite(piece.kind, piece.color, piece.state, piece.elapsed_in_state_ms);
        sprite.draw_on(frame, drawX, drawY);
    }

    // Side panels: Black on the left, White on the right - each shows its
    // own name, score, and move history as a "Time | Move" table. Plain
    // data set via setScore/setMoveLog (Stage I), decoded by
    // GraphicalApplication from the wire - never through GameSnapshot,
    // since neither score nor the move log are part of the board state
    // itself.
    const int leftPanelX  = 10;
    const int rightPanelX = ViewConfig::PANEL_WIDTH + boardW + ViewConfig::BOARD_MARGIN + PLAYER_PANEL_GAP_PX;

    drawPlayerPanel(frame, leftPanelX, "Black", blackPlayerName, blackScore, blackMoves);
    drawPlayerPanel(frame, rightPanelX, "White", whitePlayerName, whiteScore, whiteMoves);

    // Spec (Iteration 9): display an end-of-game message when game_over is true.
    if (snapshot.game_over) {
        std::string message = "Game Over";
        Chess::Color winner = survivingKingColor(snapshot);
        if (winner == Chess::Color::White) message += " - White Wins!";
        else if (winner == Chess::Color::Black) message += " - Black Wins!";

        int textX = ViewConfig::PANEL_WIDTH + boardW / 2 - 180;
        int textY = ViewConfig::BOARD_MARGIN + boardH / 2;
        frame.put_text(message, textX, textY, 1.0);
    }

    // Stage G2c: which room this is - shown in the OS window title bar
    // instead of drawn on the canvas, so it can never overlap the Black
    // panel's own header (it used to be drawn at the same corner). Set
    // once per room, not every frame - setWindowTitle needs the window to
    // already exist, which it does by the time render() is first called.
    if (!roomName.empty() && roomName != titledRoomName) {
        cv::setWindowTitle(ViewConfig::WINDOW_NAME, "Kung Fu Chess - Room " + roomName);
        titledRoomName = roomName;
    }

    // A count, not a name list (see setSpectatorCount) - drawn just below
    // the Black panel (leftPanelX, PANEL_Y..PANEL_Y+PANEL_TOTAL_H), not in
    // the top-left corner itself: that corner is where the Black panel's
    // own header sits, and the two were overlapping there.
    frame.put_text("Spectators: " + std::to_string(spectatorCount), leftPanelX, PANEL_Y + PANEL_TOTAL_H + 20, 0.6, cv::Scalar(255, 255, 255));

    // Stage D: disconnect grace-period countdown - shown in the bottom
    // margin band (reserved space below the board, otherwise empty) rather
    // than the top band, which is where the A-H file labels already live.
    if (disconnectActive) {
        int textX = ViewConfig::PANEL_WIDTH + boardW / 2 - 200;
        int textY = boardY + boardH + margin - 8;
        frame.put_text(disconnectMessage, textX, textY, 0.8, cv::Scalar(0, 0, 255));
    }

    // Drag-to-resize handle - drawn last so it's always visible on top of
    // pieces/frame near the board's bottom-right corner. Geometry (the
    // clickable square) comes from BoardScale, shared with
    // GraphicalApplication's hit-testing, so it can never drift from what's
    // drawn here. The classic "resize grip" glyph (three short parallel
    // diagonal strokes, growing outward from the corner) - matches how
    // lichess/chess.com mark their own resize handles, deliberately subtle
    // rather than a solid block.
    {
        int hx, hy;
        boardScale.handleTopLeft(snapshot.board_width, snapshot.board_height, hx, hy);
        const cv::Scalar STROKE_OUTLINE(0, 0, 0);
        const cv::Scalar STROKE_COLOR(230, 230, 230);
        int right  = hx + BoardScale::HANDLE_SIZE - 2;
        int bottom = hy + BoardScale::HANDLE_SIZE - 2;
        for (int offset : { 5, 10, 15 }) {
            frame.line(right - offset, bottom, right, bottom - offset, STROKE_OUTLINE, 2);
            frame.line(right - offset, bottom, right, bottom - offset, STROKE_COLOR, 1);
        }
    }

    // Not using Img::show() here - it blocks on waitKey(0) every call, which
    // would freeze a live render loop. GraphicalApplication owns the event
    // pump (waitKey) and calls render() once per frame instead.
    cv::imshow(ViewConfig::WINDOW_NAME, frame.get_mat());
}
