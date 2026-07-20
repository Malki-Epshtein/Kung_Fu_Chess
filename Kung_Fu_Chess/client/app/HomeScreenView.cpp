#include "HomeScreenView.h"
#include "HomeScreen.h"
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

    void onHomeMouseEvent(int event, int x, int y, int /*flags*/, void* /*userdata*/) {
        if (event != cv::EVENT_LBUTTONDOWN)
            return;
        switch (HomeScreen::hitTest(x, y)) {
            case HomeScreenChoice::Play:
                std::cout << "[client] Home: Play clicked" << std::endl;
                break;
            case HomeScreenChoice::Room:
                std::cout << "[client] Home: Room clicked" << std::endl;
                break;
            case HomeScreenChoice::None:
                break;
        }
    }
}

void runHomeScreen() {
    cv::namedWindow(kHomeWindowName);
    cv::setMouseCallback(kHomeWindowName, onHomeMouseEvent, nullptr);
    std::cout << "[client] Home screen opened" << std::endl;

    cv::Mat canvas(500, 800, CV_8UC3, cv::Scalar(40, 40, 40));
    drawButton(canvas, HomeScreen::playButtonBounds(), "Play");
    drawButton(canvas, HomeScreen::roomButtonBounds(), "Room");

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
    }
    cv::destroyWindow(kHomeWindowName);
}
