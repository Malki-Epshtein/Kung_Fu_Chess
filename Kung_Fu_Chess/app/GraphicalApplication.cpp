#include "GraphicalApplication.h"
#include "../view/ViewConfig.h"
#include <opencv2/opencv.hpp>

namespace {
    void onMouseEvent(int event, int x, int y, int /*flags*/, void* userdata) {
        if (event != cv::EVENT_LBUTTONDOWN)
            return;
        auto* controller = static_cast<Controller*>(userdata);
        // The board is now drawn centered, offset right by the left panel's
        // width and down by the top frame margin (see ViewConfig::PANEL_WIDTH
        // / BOARD_MARGIN) - correct raw window coordinates back to
        // board-local ones here, at the OS input boundary, so
        // BoardMapper/Controller stay unaware of any rendering layout.
        // A click landing inside a side panel or the frame margin becomes a
        // negative coordinate, which Controller's existing bounds check
        // already treats as out-of-board.
        controller->handleMouseClick(x - ViewConfig::PANEL_WIDTH, y - ViewConfig::BOARD_MARGIN);
    }
}

void GraphicalApplication::run() {
    cv::namedWindow(ViewConfig::WINDOW_NAME);
    cv::setMouseCallback(ViewConfig::WINDOW_NAME, onMouseEvent, &controller);

    while (true) {
        controller.handleWait(ViewConfig::GAME_MS_PER_FRAME);
        view.render(controller.getSnapshot());

        int key = cv::waitKey(30);
        if (key == 27) // ESC
            break;
        if (cv::getWindowProperty(ViewConfig::WINDOW_NAME, cv::WND_PROP_VISIBLE) < 1)
            break;
    }
}
