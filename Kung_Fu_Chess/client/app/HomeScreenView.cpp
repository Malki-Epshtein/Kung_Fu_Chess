#include "HomeScreenView.h"
#include "HomeScreen.h"
#include "RoomDialog.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

namespace {
    constexpr const char* kHomeWindowName = "Kung Fu Chess - Home";

    void drawButton(cv::Mat& canvas, const ButtonBounds& bounds, const std::string& label) {
        cv::Rect rect(bounds.x, bounds.y, bounds.width, bounds.height);
        cv::rectangle(canvas, rect, cv::Scalar(200, 200, 200), cv::FILLED);
        cv::rectangle(canvas, rect, cv::Scalar(80, 80, 80), 2);
        cv::putText(canvas, label, cv::Point(bounds.x + 20, bounds.y + bounds.height / 2 + 8),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
    }

    // Set on the mouse callback, consumed by the loop in runHomeScreen()
    // below - deliberately NOT acted on inside the callback itself.
    // showRoomDialog() runs its own nested Win32 message loop, and calling
    // it from inside OpenCV's own mouse-event dispatch nests two message
    // loops on the same thread, which is fragile (observed: the dialog
    // closing itself immediately). Deferring the actual handling to a
    // clean point between cv::waitKey() calls avoids that entirely.
    HomeScreenChoice g_pendingChoice = HomeScreenChoice::None;

    void onHomeMouseEvent(int event, int x, int y, int /*flags*/, void* /*userdata*/) {
        if (event != cv::EVENT_LBUTTONDOWN)
            return;
        HomeScreenChoice choice = HomeScreen::hitTest(x, y);
        if (choice != HomeScreenChoice::None)
            g_pendingChoice = choice;
    }
}

void runHomeScreen() {
    cv::namedWindow(kHomeWindowName);
    cv::setMouseCallback(kHomeWindowName, onHomeMouseEvent, nullptr);
    std::cout << "[client] Home screen opened" << std::endl;

    cv::Mat canvas(500, 800, CV_8UC3, cv::Scalar(40, 40, 40));
    drawButton(canvas, HomeScreen::playButtonBounds(), "Play");
    drawButton(canvas, HomeScreen::roomButtonBounds(), "Room");

    g_pendingChoice = HomeScreenChoice::None;
    while (true) {
        cv::imshow(kHomeWindowName, canvas);
        int key = cv::waitKey(30);
        if (key == 27) { // ESC
            std::cout << "[client] Home screen closed: ESC pressed" << std::endl;
            break;
        }
        double visible = cv::getWindowProperty(kHomeWindowName, cv::WND_PROP_VISIBLE);
        if (visible < 1) {
            std::cout << "[client] Home screen closed: window no longer visible" << std::endl;
            break;
        }

        if (g_pendingChoice == HomeScreenChoice::Play) {
            std::cout << "[client] Home: Play clicked" << std::endl;
            g_pendingChoice = HomeScreenChoice::None;
        } else if (g_pendingChoice == HomeScreenChoice::Room) {
            std::cout << "[client] Home: Room clicked" << std::endl;
            g_pendingChoice = HomeScreenChoice::None;
            showRoomDialog();
        }
    }
    cv::destroyWindow(kHomeWindowName);
}
