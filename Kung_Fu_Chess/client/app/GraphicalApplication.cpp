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
    // Runs on WsClient's network thread. The server sends three
    // differently-shaped JSON payloads on the same connection: a broadcast
    // GameSnapshot (un-enveloped, has "board_width"), a direct
    // command-reply ack (un-enveloped, has "success"), and an enveloped
    // server->client push (has "type") like MOVE_LOGGED. Only the first two
    // are handled elsewhere/below; MOVE_LOGGED is handled first here since
    // it would otherwise be silently dropped by the "board_width" check
    // below (it has neither key).
    try {
        nlohmann::json j = nlohmann::json::parse(text);

        if (j.value("type", std::string()) == "MOVE_LOGGED") {
            MoveLogEvent event = MoveLogCodec::decode(j.at("payload"));
            std::lock_guard<std::mutex> lock(snapshotMutex);
            if (event.color == Chess::Color::White)
                networkWhiteMoves.push_back(event.entry);
            else if (event.color == Chess::Color::Black)
                networkBlackMoves.push_back(event.entry);
            return;
        }

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

        // Stage I: score rides along the same way disconnect does.
        int whiteScore = j.value("score", nlohmann::json::object()).value("white", 0);
        int blackScore = j.value("score", nlohmann::json::object()).value("black", 0);

        std::lock_guard<std::mutex> lock(snapshotMutex);
        networkSnapshot = std::move(decoded);
        networkDisconnectActive = active;
        networkDisconnectMessage = std::move(message);
        networkWhiteScore = whiteScore;
        networkBlackScore = blackScore;
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

    // One-time backfill for a room already in progress (Stage I) - safe to
    // touch networkWhiteMoves/networkBlackMoves without the lock here,
    // since onMessage isn't installed yet (a couple of lines below).
    MoveLogBundle backfill = MoveLogCodec::decodeAll(home.moveLog);
    networkWhiteMoves = std::move(backfill.white);
    networkBlackMoves = std::move(backfill.black);

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
        int         whiteScore, blackScore;
        std::vector<MoveEntry> whiteMoves, blackMoves;
        {
            std::lock_guard<std::mutex> lock(snapshotMutex);
            latestSnapshot = networkSnapshot;
            disconnectActive = networkDisconnectActive;
            disconnectMessage = networkDisconnectMessage;
            whiteScore = networkWhiteScore;
            blackScore = networkBlackScore;
            whiteMoves = networkWhiteMoves;
            blackMoves = networkBlackMoves;
        }
        view.setDisconnectStatus(disconnectActive, disconnectMessage);
        view.setScore(whiteScore, blackScore);
        view.setMoveLog(std::move(whiteMoves), std::move(blackMoves));
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
