#include "GraphicalApplication.h"
#include <opencv2/opencv.hpp>

namespace {
    void onMouseEvent(int event, int x, int y, int /*flags*/, void* userdata) {
        if (event != cv::EVENT_LBUTTONDOWN)
            return;
        auto* controller = static_cast<Controller*>(userdata);
        // The board is now drawn centered, offset right by the left panel's
        // width (see ImageView::PANEL_WIDTH) - correct raw window coordinates
        // back to board-local ones here, at the OS input boundary, so
        // BoardMapper/Controller stay unaware of any rendering layout.
        // A click landing inside a side panel becomes a negative x, which
        // Controller's existing bounds check already treats as out-of-board.
        controller->handleMouseClick(x - ImageView::PANEL_WIDTH, y);
    }
}

void GraphicalApplication::run() {
    cv::namedWindow(ImageView::WINDOW_NAME);
    cv::setMouseCallback(ImageView::WINDOW_NAME, onMouseEvent, &controller);

    // Roughly 1:1 with real time, so one board cell (1000 game-ms, set in
    // MotionPath.cpp) takes about one real second to cross.
    const int GAME_MS_PER_FRAME = 30;

    while (true) {
        controller.handleWait(GAME_MS_PER_FRAME);
        view.render(controller.getSnapshot());

        int key = cv::waitKey(30);
        if (key == 27) // ESC
            break;
        if (cv::getWindowProperty(ImageView::WINDOW_NAME, cv::WND_PROP_VISIBLE) < 1)
            break;
    }
}
