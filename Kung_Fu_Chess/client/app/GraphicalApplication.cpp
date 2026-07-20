#include "GraphicalApplication.h"
#include "HomeScreenView.h"
#include "../view/ViewConfig.h"
#include "../../shared/protocol/GameSnapshotCodec.h"
#include <opencv2/opencv.hpp>
#include <iostream>

namespace {
    // "White"/"Black" -> the other color's display name, for the
    // disconnect-countdown message (Stage D).
    const char* opponentName(const std::string& color) {
        return color == "White" ? "Black" : "White";
    }

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

void GraphicalApplication::onMessage(const std::string& text) {
    // Runs on WsClient's network thread. The server sends two
    // differently-shaped, un-enveloped JSON payloads on the same connection:
    // a broadcast GameSnapshot (has "board_width") and a direct
    // command-reply ack (has "success"). Only the former is meaningful to
    // the view - and only networkSnapshot (never latestSnapshot) is touched
    // here, under the lock; run() copies it to latestSnapshot on the GUI
    // thread once per frame.
    try {
        nlohmann::json j = nlohmann::json::parse(text);
        if (!j.contains("board_width"))
            return;

        GameSnapshot decoded = GameSnapshotCodec::decode(j);

        // Stage D: the "disconnect" key rides along on the same broadcast
        // rather than being a separate message shape - present only while a
        // seated player's grace-period countdown is active.
        bool active = false;
        std::string message;
        if (j.contains("disconnect") && j["disconnect"].value("active", false)) {
            std::string color = j["disconnect"].value("color", "");
            int secondsRemaining = j["disconnect"].value("secondsRemaining", 0);
            active = true;
            message = secondsRemaining > 0
                ? color + " disconnected - auto-resign in " + std::to_string(secondsRemaining) + "s"
                : color + " resigned (disconnected) - " + opponentName(color) + " wins!";
        }

        std::lock_guard<std::mutex> lock(snapshotMutex);
        networkSnapshot = std::move(decoded);
        networkDisconnectActive = active;
        networkDisconnectMessage = std::move(message);
    } catch (const std::exception& e) {
        std::cerr << "[client] failed to parse server message: " << e.what() << std::endl;
    }
}

void GraphicalApplication::run() {
    // Shown first; blocks until a room is actually created/joined (Play
    // isn't wired to anything yet - Stage H) or the user closes the
    // window, which quits the app the same way closing the game window
    // does today - there's nothing useful to show without a room.
    HomeScreenResult home = runHomeScreen(client);//כל מסך הבית קורה עעכשיו
    if (!home.joinedRoom) {
        std::cout << "[client] exiting: Home screen closed without joining a room" << std::endl;
        return;
    }
    view.setRoomName(home.roomName);//שומרים את שם החדר שהמשתמש הצטרף אליו כדי להציג אותו על המסך

    // Only installed now that a room is actually joined - runHomeScreen()
    // needed the connection free for its own blocking CreateRoom/JoinRoom
    // exchange (same reasoning as LoginFlow, in the constructor).
    client.setOnMessage([this](const std::string& text) { onMessage(text); });//אני מחכה עש שיהיה לי תשובה

    cv::namedWindow(ViewConfig::WINDOW_NAME);
    cv::setMouseCallback(ViewConfig::WINDOW_NAME, onMouseEvent, &controller);
    std::cout << "[client] window opened, entering render loop" << std::endl;

    int frame = 0;
    while (true) {
        bool        disconnectActive;
        std::string disconnectMessage;
        {
            std::lock_guard<std::mutex> lock(snapshotMutex);
            latestSnapshot = networkSnapshot;
            disconnectActive = networkDisconnectActive;
            disconnectMessage = networkDisconnectMessage;
        }
        view.setDisconnectStatus(disconnectActive, disconnectMessage);
        view.render(controller.getSnapshot());
        ++frame;

        int key = cv::waitKey(30);
        if (key == 27) { // ESC
            std::cout << "[client] exiting: ESC pressed at frame " << frame << std::endl;
            break;
        }
        double visible = cv::getWindowProperty(ViewConfig::WINDOW_NAME, cv::WND_PROP_VISIBLE);
        if (visible < 1) {
            std::cout << "[client] exiting: window no longer visible (property=" << visible
                       << ") at frame " << frame << std::endl;
            break;
        }
    }
}
