#pragma once

// Single source of truth for the View layer's own constants - pixel sizes,
// window title, render cadence. Deliberately does NOT include game-rule
// timing (e.g. RealTimeArbiter's rest durations) - those belong to a
// different layer's ownership per the architecture's layering rules, and
// mixing them in here would let the view reach into data it doesn't own.
namespace ViewConfig {
    constexpr const char* WINDOW_NAME = "Kung Fu Chess";
    constexpr int PANEL_WIDTH       = 280;
    constexpr int BOARD_WIDTH_PX    = 800;
    constexpr int CELL_SIZE         = 100;
    constexpr int GAME_MS_PER_FRAME = 30;
    // Vertical strip above AND below the board reserved for the frame/
    // coordinate labels, so they never have to overlap the board itself.
    constexpr int BOARD_MARGIN      = 30;
}
