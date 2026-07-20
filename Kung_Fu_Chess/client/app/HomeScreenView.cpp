#include "HomeScreenView.h"
#include "HomeScreen.h"
#include "RoomDialog.h"
#include "../net/BlockingRequest.h"
#include "../../shared/protocol/Message.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

namespace {
    constexpr const char* kHomeWindowName = "Kung Fu Chess - Home";

    // Set on the mouse callback, consumed by the loop in runHomeScreen()
    // below - deliberately NOT acted on inside the callback itself.
    // showRoomDialog() runs its own nested Win32 message loop, and calling
    // it from inside OpenCV's own mouse-event dispatch nests two message
    // loops on the same thread, which is fragile (observed: the dialog
    // closing itself immediately). Deferring the actual handling to a
    // clean point between cv::waitKey() calls avoids that entirely.
    HomeScreenChoice g_pendingChoice = HomeScreenChoice::None;

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
        HomeScreenChoice choice = HomeScreen::hitTest(x, y);
        if (choice != HomeScreenChoice::None)
            g_pendingChoice = choice;
    }

    // Sends CreateRoom/JoinRoom for `roomName` and blocks for the reply.
    // Empty on failure (message logged); non-empty is the room name to
    // display, taken from the server's reply rather than echoing back
    // whatever was typed, since the two can differ in principle.
    std::string sendRoomRequest(WsClient& client, RoomDialogResult::Action action, const std::string& roomName) {
        MessageType type = (action == RoomDialogResult::Action::Create) ? MessageType::CreateRoom : MessageType::JoinRoom;
        Message request{ type, { {"name", roomName} } };
        nlohmann::json reply = sendAndWaitForReply(client, request);

        if (!reply.value("success", false)) {
            std::string message = reply.value("message", "");
            std::cout << "[client] Room request failed: " << message << std::endl;
            showRoomError(message);
            return {};
        }
        return reply.value("roomName", roomName);
    }
}

HomeScreenResult runHomeScreen(WsClient& client) {
    cv::namedWindow(kHomeWindowName);
    cv::setMouseCallback(kHomeWindowName, onHomeMouseEvent, nullptr);
    std::cout << "[client] Home screen opened" << std::endl;

    cv::Mat canvas(500, 800, CV_8UC3, cv::Scalar(40, 40, 40));
    drawButton(canvas, HomeScreen::playButtonBounds(), "Play");
    drawButton(canvas, HomeScreen::roomButtonBounds(), "Room");

    HomeScreenResult result;
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

            RoomDialogResult dialogResult = showRoomDialog();
            // Cancel, or Create/Join with nothing typed, is a no-op back
            // to the Home screen - matches the plan's Cancel/empty-field
            // behavior exactly.
            if (dialogResult.action == RoomDialogResult::Action::Cancel || dialogResult.roomName.empty())
                continue;

            std::string joinedName = sendRoomRequest(client, dialogResult.action, dialogResult.roomName);
            if (!joinedName.empty()) {
                result.joinedRoom = true;
                result.roomName   = joinedName;
                break;
            }
            // Failure (e.g. "room already exists") already logged inside
            // sendRoomRequest() - stay on the Home screen so the user can
            // try again via the Room button.
        }
    }
    cv::destroyWindow(kHomeWindowName);
    return result;
}
