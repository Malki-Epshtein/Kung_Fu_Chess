#pragma once
#include "ViewConfig.h"
#include <algorithm>

// The one piece of ViewConfig-adjacent state that's genuinely mutable at
// runtime - everything else in ViewConfig stays a real compile-time
// constant (see that header's own comment: window title, frame timing,
// panel/margin widths). Owned by GraphicalApplication, shared by const
// reference with Controller (bounds check + click-to-cell mapping) and
// ImageView (all board/piece/label pixel math) - the same pattern
// GraphicalApplication already uses to share latestSnapshot with Controller.
//
// Defaults to ViewConfig::CELL_SIZE so Phase 1 (routing every read through
// BoardScale instead of the constexpr directly) is behavior-preserving by
// default - the same size as before, just no longer hardcoded at compile
// time. Phase 3 wires up the actual drag handle that lets a player change
// this live (see C:\Users\This User\.claude\plans\curious-moseying-beaver.md).
class BoardScale {
public:
    static constexpr int MIN_CELL_SIZE = 60;
    static constexpr int MAX_CELL_SIZE = 120;

    void setCellSize(int size) {
        cellSize_ = std::clamp(size, MIN_CELL_SIZE, MAX_CELL_SIZE);
    }

    int cellSize() const { return cellSize_; }

    int boardPixelWidth(int cols) const  { return cols * cellSize_; }
    int boardPixelHeight(int rows) const { return rows * cellSize_; }

    // The drag-to-resize handle's geometry (a HANDLE_SIZE square at the
    // board's bottom-right corner, in full-window pixel coordinates) - kept
    // here rather than duplicated between ImageView (draws it) and
    // GraphicalApplication (hit-tests clicks/drags against it), so both
    // always agree on where it is.
    static constexpr int HANDLE_SIZE = 18;

    void handleTopLeft(int cols, int rows, int& x, int& y) const {
        x = ViewConfig::PANEL_WIDTH + boardPixelWidth(cols) - HANDLE_SIZE;
        y = ViewConfig::BOARD_MARGIN + boardPixelHeight(rows) - HANDLE_SIZE;
    }

    bool isInHandle(int cols, int rows, int px, int py) const {
        int x, y;
        handleTopLeft(cols, rows, x, y);
        return px >= x && px < x + HANDLE_SIZE && py >= y && py < y + HANDLE_SIZE;
    }

private:
    int cellSize_ = ViewConfig::CELL_SIZE;
};
